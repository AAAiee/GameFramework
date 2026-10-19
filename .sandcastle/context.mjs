import { createHash, randomUUID } from "node:crypto";
import {
  existsSync,
  mkdirSync,
  readFileSync,
  realpathSync,
  statSync,
  writeFileSync,
} from "node:fs";
import { basename, join, sep } from "node:path";
import { createWorkflowPacket } from "@rebuild/sandcastle/workflow";

export const digest = (value) =>
  createHash("sha256").update(JSON.stringify(value)).digest("hex");

/** Copies only explicit current evidence into the ignored checkout mount; no sessions, credentials or history. */
export function createEditorContext({ sandbox, runtime, task, context, git }) {
  if (
    !context ||
    typeof context.scope !== "string" ||
    !context.scope.trim() ||
    typeof context.base !== "string" ||
    !/^[a-f0-9]{40}$/.test(context.base) ||
    ![context.entrypoints, context.commands, context.references].every(
      (value) =>
        Array.isArray(value) &&
        value.every((entry) => typeof entry === "string" && entry.trim()),
    )
  )
    throw new Error(
      "Explicit scope, immutable base, entrypoints, commands and references are required for stage context.",
    );
  git(sandbox.worktreePath, "cat-file", "-e", `${context.base}^{commit}`);
  const relative = `.sandcastle/context/${runtime.key}`;
  const host = join(sandbox.worktreePath, relative);
  const persisted = join(runtime.directory, "stage-context");
  git(sandbox.worktreePath, "check-ignore", "-q", `${relative}/packet.json`);
  mkdirSync(host, { recursive: true });
  mkdirSync(persisted, { recursive: true });
  if (!realpathSync(host).startsWith(realpathSync(sandbox.worktreePath) + sep))
    throw new Error(
      "Stage context must remain inside the ignored task checkout.",
    );
  if (
    !realpathSync(persisted).startsWith(realpathSync(runtime.directory) + sep)
  )
    throw new Error(
      "Retained context must remain inside this run's directory.",
    );
  const write = (name, value) => {
    const path = `${relative}/${name}`;
    const retained = join(persisted, name);
    if (existsSync(retained)) {
      if (!readFileSync(retained).equals(Buffer.from(value)))
        throw new Error(
          "Existing context evidence is immutable and was preserved.",
        );
    } else writeFileSync(retained, value, { flag: "wx" });
    writeFileSync(join(sandbox.worktreePath, path), value);
    return path;
  };
  const restore = (pointer) => {
    if (
      !pointer.startsWith(relative + "/") ||
      /[\\/]/.test(pointer.slice(relative.length + 1))
    )
      return;
    const name = pointer.slice(relative.length + 1);
    const retained = join(persisted, name);
    if (existsSync(retained)) write(name, readFileSync(retained));
  };
  const explicitFiles = new Set(
    context.references.flatMap((source) => {
      const path = existsSync(source)
        ? source
        : join(sandbox.worktreePath, source);
      return existsSync(path) && statSync(path).isFile()
        ? [realpathSync(path)]
        : [];
    }),
  );
  const evidence = (source) => {
    const copied = source.match(/ \(sandbox copy: (.+)\)$/);
    if (copied) {
      restore(copied[1]);
      return source;
    }
    restore(source);
    const path = existsSync(source)
      ? source
      : join(sandbox.worktreePath, source);
    if (!existsSync(path) || !statSync(path).isFile()) return source;
    const actual = realpathSync(path);
    if (
      !actual.startsWith(realpathSync(sandbox.worktreePath) + sep) &&
      !actual.startsWith(realpathSync(runtime.directory) + sep) &&
      !explicitFiles.has(actual)
    )
      throw new Error(
        "Raw evidence is outside the explicit project/run context roots.",
      );
    if (
      /(?:^|[\\/])(?:\.env(?:\.[^\\/]*)?|auth\.json|credentials(?:\.json)?|sessions?)(?:[\\/]|$)/i.test(
        actual,
      )
    )
      throw new Error("Credential and session files cannot be stage evidence.");
    const bytes = readFileSync(path);
    const name = basename(path).replace(/[^a-zA-Z0-9._-]/g, "_");
    const accessible = write(
      `evidence-${createHash("sha256").update(bytes).digest("hex")}-${name}`,
      bytes,
    );
    return `${source} (sandbox copy: ${accessible})`;
  };
  const references = context.references.map(evidence);
  const identity = { ...context, references };
  const requirements = `${relative}/requirements-${digest({ task, context: identity })}.json`;
  const initial = createWorkflowPacket({
    stage: "implement",
    task,
    ...context,
    branch: sandbox.branch,
    references: [requirements, ...references],
  });
  write(basename(requirements), initial);
  return {
    get requirements() {
      return write(basename(requirements), initial);
    },
    initial,
    evidence,
    current: () => ({
      ...context,
      references: context.references.map(evidence),
    }),
    retained(pointer) {
      const name = pointer.slice(relative.length + 1);
      if (
        !pointer.startsWith(relative + "/") ||
        /[\\/]/.test(name) ||
        !existsSync(join(persisted, name))
      )
        throw new Error(
          "The stage context pointer does not identify retained evidence for this run.",
        );
      return { accessible: pointer, host: join(persisted, name) };
    },
    invocation(stage, workflow) {
      const name = `${stage}-generation-${workflow.evidenceGeneration ?? 0}-implementation-${workflow.attempt?.implementation ?? 1}-transport-${workflow.attempt?.transport ?? 1}-${randomUUID()}`;
      return {
        name,
        log: join(runtime.directory, `${name}.log`),
        write: (suffix, value) => write(`${name}-${suffix}`, value),
      };
    },
  };
}
