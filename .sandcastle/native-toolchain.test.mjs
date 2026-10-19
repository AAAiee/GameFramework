import { createEditorEvidence } from "./evidence.mjs";
import { runWorkflow } from "@rebuild/sandcastle/workflow";
import test from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
const msbuild =
  "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe";
const installed =
  "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/";
test(
  "native environment binds the effective compiler selection rather than the newest installed toolset",
  {
    skip:
      process.platform !== "win32" ||
      !existsSync(msbuild) ||
      !existsSync(installed + "14.38.33130") ||
      !existsSync(installed + "14.44.35207"),
  },
  async () => {
    const inspect = (version) =>
      JSON.parse(
        execFileSync(
          "pwsh.exe",
          [
            "-NoProfile",
            "-File",
            fileURLToPath(
              new URL(
                "../Scripts/Inspect-NativeEnvironment.ps1",
                import.meta.url,
              ),
            ),
            "-Worktree",
            fileURLToPath(new URL("..", import.meta.url)),
            "-MSBuild",
            msbuild,
          ],
          {
            encoding: "utf8",
            env: { ...process.env, VCToolsVersion: version },
            windowsHide: true,
          },
        ),
      );
    const older = inspect("14.38.33130");
    const newer = inspect("14.44.35207");
    assert.match(older.compiler.path, /14\.38\.33130/);
    assert.match(newer.compiler.path, /14\.44\.35207/);
    assert.notDeepEqual(
      older,
      newer,
      "changing the effective selection must invalidate environment evidence",
    );
    let current = older;
    const events = [];
    const project = {
      id: "compiler-selection",
      config: { worktree: fileURLToPath(new URL("..", import.meta.url)) },
    };
    const options = {
      project,
      recovery: {},
      evidence: createEditorEvidence({
        validationPolicy: async () => "compiler-probe-v1",
        dependencies: async () => "unchanged-pins",
        environment: async () => JSON.stringify(current),
        reviewPolicy: async () => "same-review",
      }),
      operations: {
        selectTask: async () => ({
          id: "compiler",
          criteria: [{ id: "build", description: "uses actual compiler" }],
        }),
        implement: async () => {
          events.push("implement");
          return "unchanged-code";
        },
        currentCandidate: async () => "unchanged-code",
        validate: async ({ candidate }) => {
          events.push("validate");
          return {
            candidate,
            passed: true,
            evidence: ["deterministic fixture, no native acceptance claim"],
          };
        },
        review: async ({ candidate }) => {
          events.push("review");
          return {
            candidate,
            approved: true,
            criteria: [
              { id: "build", satisfied: true, evidence: "fixture review" },
            ],
          };
        },
        deliver: async ({ candidate }) => {
          events.push("deliver");
          return { candidate, reference: "fixture" };
        },
        save: async () => {},
      },
    };
    // Evidence discovery reads committed dependency metadata for a real candidate.
    const candidate = execFileSync("git", ["rev-parse", "HEAD"], {
      cwd: project.config.worktree,
      encoding: "utf8",
      windowsHide: true,
    }).trim();
    options.operations.implement = async () => {
      events.push("implement");
      return candidate;
    };
    options.operations.currentCandidate = async () => candidate;
    const checkpoint = JSON.parse(JSON.stringify(await runWorkflow(options)));
    events.length = 0;
    current = newer;
    await runWorkflow({ ...options, checkpoint });
    assert.deepEqual(
      events,
      ["validate", "review", "deliver"],
      "changed compiler selection rechecks unchanged code without implementation",
    );
  },
);
