import { join } from "node:path";
import {
  createCommandRunner,
  createGitHubReader,
  discoverWindowsProxy,
  findExecutable,
  proxyEnvironment,
} from "@rebuild/sandcastle/workflow/integrations";

export function issueCriteria(body) {
  const lines = String(body ?? "").split(/\r?\n/);
  const start = lines.findIndex((line) =>
    /^##[ \t]+Acceptance criteria[ \t]*$/i.test(line),
  );
  if (start === -1) return [];
  const criteria = [];
  for (const line of lines.slice(start + 1)) {
    if (/^#{1,2}[ \t]/.test(line)) break;
    const item = line.match(/^[ \t]*-[ \t]+\[[ xX]\][ \t]+(.+)$/);
    if (item)
      criteria.push({
        id: `C${criteria.length + 1}`,
        description: item[1].trim(),
      });
    else if (/^[ \t]+\S/.test(line) && criteria.length)
      criteria.at(-1).description += `\n${line.trim()}`;
  }
  return criteria;
}

/** Tracker eligibility only. Local handoffs and live execution remain separate increments. */
export async function inspectQueue(config, { reader, signal } = {}) {
  if (!reader) {
    const proxy = config.discoverWindowsProxy
      ? await discoverWindowsProxy()
      : undefined;
    const githubEnvironment = proxyEnvironment(process.env, proxy);
    const gh =
      config.tools?.gh ??
      findExecutable("gh", {
        candidates:
          process.platform === "win32" && process.env.ProgramFiles
            ? [join(process.env.ProgramFiles, "GitHub CLI/gh.exe")]
            : [],
      });
    const command = createCommandRunner({
      cwd: process.cwd(),
      tools: { gh, ...config.tools },
      githubEnvironment,
      timeoutMs: config.commandTimeoutMs,
    });
    reader = createGitHubReader({
      repository: config.repository,
      command,
      signal: signal ?? AbortSignal.timeout(config.discoveryTimeoutMs ?? 45000),
    });
  }
  const issues = await reader.issues(config.queueLabels);
  return {
    repository: config.repository,
    mode: "read-only queue inspection",
    execution: "disabled while the pipeline is rebuilt",
    criteria:
      "GitHub open state, labels, native dependencies, parent/assignee status and acceptance criteria; local handoffs are not inferred",
    issues: issues
      .filter((issue) => !issue.pull_request)
      .sort((a, b) => a.number - b.number)
      .map((issue) => {
        const criteria = issueCriteria(issue.body);
        let reason;
        if (issue.state !== "open") reason = "closed";
        else if (issue.sub_issues_summary?.total > 0)
          reason = "parent specification";
        else if (issue.assignees?.length) reason = "assigned";
        else if (
          !Number.isSafeInteger(issue.issue_dependencies_summary?.blocked_by) ||
          !Number.isSafeInteger(issue.sub_issues_summary?.total)
        )
          reason = "dependency metadata unavailable";
        else if (issue.issue_dependencies_summary.blocked_by > 0)
          reason = `${issue.issue_dependencies_summary.blocked_by} open dependencies`;
        else if (!criteria.length) reason = "no checkable acceptance criteria";
        return {
          number: issue.number,
          title: issue.title,
          eligibleOnGitHub: !reason,
          reason: reason ?? "no tracker blockers",
          criteria: criteria.length,
          url: issue.html_url,
        };
      }),
  };
}
