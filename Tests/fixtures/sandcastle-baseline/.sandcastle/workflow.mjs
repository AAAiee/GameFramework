import { createHash } from "node:crypto";

export const hash = (text) => createHash("sha256").update(text).digest("hex");

export function criteria(body) {
  return [...body.matchAll(/^\s*- \[[ xX]\]\s+(.+)$/gm)].map((match, i) => ({
    id: `C${i + 1}`, text: match[1],
  }));
}

export function selectIssue(config, issues, active) {
  const byNumber = new Map(issues.map((issue) => [issue.number, issue]));
  for (const item of config.issues) {
    if (!byNumber.has(item.number)) throw new Error(`GitHub did not return #${item.number}.`);
  }
  if (active) {
    const issue = byNumber.get(active.number);
    if (!issue) throw new Error(`Unknown active issue #${active.number}.`);
    if (issue.state === "closed" && !["merged", "closed"].includes(active.phase)) {
      throw new Error(`#${active.number} was closed externally before this run merged it.`);
    }
    return issue;
  }
  return config.issues.map((item) => ({ item, issue: byNumber.get(item.number) }))
    .find(({ item, issue }) => issue.state === "open" &&
      item.dependencies.every((number) => byNumber.get(number)?.state === "closed") &&
      (issue.labels.some((label) => (label.name ?? label) === "Sandcastle") ||
        item.number === 12 || item.number === 25))?.issue;
}

export function validationPlan(config, number, completed) {
  const current = config.issues.find((issue) => issue.number === number);
  if (!current) throw new Error(`Issue #${number} is outside spec #${config.parent}.`);
  const included = config.issues.filter((issue) => issue.number === number || completed.includes(issue.number));
  return {
    issue: number,
    selectors: [...new Set(included.flatMap((issue) => issue.selectors))],
    windowsChecks: (number === 25 ? included : [current]).filter((issue) => issue.ui).map((issue) => ({
      script: `Tests/Windows/issue-${issue.number}.ps1`,
      marker: `PASS: editor-issue-${issue.number}`,
    })),
  };
}

export function parseReview(stdout, issue, sha) {
  const matches = [...stdout.matchAll(/<review>([\s\S]*?)<\/review>/g)];
  if (!matches.length) throw new Error("Reviewer did not return a <review> result.");
  const review = JSON.parse(matches.at(-1)[1]);
  const required = criteria(issue.body);
  if (!required.length) throw new Error("Issue has no checkable acceptance criteria.");
  if (review.approved !== true || review.sha !== sha || review.issue !== issue.number) {
    throw new Error(`Review rejected or does not identify the final commit: ${JSON.stringify(review)}`);
  }
  if (!Array.isArray(review.criteria) || review.criteria.length !== required.length ||
      new Set(review.criteria.map((entry) => entry.id)).size !== required.length) {
    throw new Error("Review must cover every acceptance criterion exactly once.");
  }
  for (const criterion of required) {
    const entry = review.criteria.find((item) => item.id === criterion.id);
    if (!entry || entry.satisfied !== true || typeof entry.evidence !== "string" || !entry.evidence.trim()) {
      throw new Error(`Reviewer has not accounted for ${criterion.id}: ${criterion.text}`);
    }
  }
  return review;
}

// IO is supplied by the Windows runner; tests exercise the same state machine.
export async function runIssue(active, ports, maxAttempts) {
  if (!["validated", "merged", "closed"].includes(active.phase)) {
    while (active.attempt < maxAttempts || active.phase === "review") {
      try {
        if (active.phase !== "review") {
          active.attempt++;
          active.phase = "implement";
          active.review = null;
          active.validation = null;
          await ports.save();
          await ports.implement(active);
          active.sha = await ports.candidate(active);
          active.validation = await ports.validate(active);
          if (active.validation.status !== "passed" || active.validation.commit !== active.sha) {
            throw new Error("Validation did not pass for the reviewed commit.");
          }
          active.phase = "review";
          await ports.save();
        }
        if (active.validation?.status !== "passed" || active.validation.commit !== active.sha) {
          throw new Error("Validation did not pass for the reviewed commit.");
        }
        active.review = await ports.review(active);
        if (active.review.sha !== active.sha) {
          throw new Error("Validation did not pass for the reviewed commit.");
        }
        active.phase = "validated";
        active.feedback = "";
        await ports.save();
        break;
      } catch (error) {
        active.feedback = error.message;
        active.phase = "repair";
        await ports.save();
        if (active.attempt >= maxAttempts) throw error;
      }
    }
    if (active.phase !== "validated") {
      throw new Error(`Repair limit reached for #${active.number}; saved feedback: ${active.feedback}`);
    }
  }
  if (active.phase === "validated") {
    await ports.merge(active);
    active.phase = "merged";
    await ports.save();
  }
  if (active.phase === "merged") {
    await ports.publish(active);
    await ports.close(active);
    active.phase = "closed";
    await ports.save();
  }
}
