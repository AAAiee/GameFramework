import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";

const digest = (value) =>
  createHash("sha256").update(JSON.stringify(value)).digest("hex");
const file = (relative) =>
  readFileSync(new URL(relative, import.meta.url), "utf8");
const git = (cwd, ...args) =>
  execFileSync("git", args, {
    cwd,
    encoding: "utf8",
    windowsHide: true,
  }).trim();
const required = async (name, discover, context) => {
  if (typeof discover !== "function")
    throw new Error(`Current ${name} discovery is required.`);
  const value = await discover(context);
  if (typeof value !== "string" || !value.trim())
    throw new Error(
      `Current ${name} is unavailable; evidence cannot be reused.`,
    );
  return value;
};

/** Project identity discovery. Native policy, actual prepared pins and toolchain/graphics observations are explicit callbacks, with no unavailable-value defaults. */
export function createEditorEvidence({
  validationPolicy,
  dependencies,
  environment,
  reviewPolicy,
}) {
  return {
    validation: async (context) => {
      const worktree = context.project.config.worktree;
      const files = git(
        worktree,
        "ls-tree",
        "-r",
        "--name-only",
        context.candidate,
      ).split("\n");
      const lock = files.includes("package-lock.json")
        ? git(worktree, "show", `${context.candidate}:package-lock.json`)
        : null;
      return {
        policy: digest({
          adapter: file("./task.mjs"),
          policy: await required(
            "validation policy",
            validationPolicy,
            context,
          ),
        }),
        dependencies: digest({
          candidateLock: lock,
          gitlinks: git(worktree, "ls-tree", "-r", context.candidate)
            .split("\n")
            .filter((entry) => entry.startsWith("160000 ")),
          runnerLock: file("../package-lock.json"),
          provenance: file("./package-provenance.json"),
          installedWorkflow: readFileSync(
            new URL(import.meta.resolve("@rebuild/sandcastle/workflow")),
            "utf8",
          ),
          actual: await required("dependencies", dependencies, context),
        }),
        environment: digest({
          node: process.version,
          platform: process.platform,
          arch: process.arch,
          actual: await required("environment", environment, context),
        }),
      };
    },
    review: async (context) =>
      digest({
        adapter: file("./task.mjs"),
        policy: await required("review policy", reviewPolicy, context),
      }),
  };
}
