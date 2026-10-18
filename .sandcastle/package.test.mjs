import test from "node:test";
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import {
  copyFileSync,
  mkdtempSync,
  readFileSync,
  realpathSync,
  rmSync,
  readdirSync,
  writeFileSync,
} from "node:fs";
import { tmpdir } from "node:os";
import { basename, join, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

test("the pinned archive isolates independent consumers A then B without Editor or credentials", (t) => {
  const temporary = mkdtempSync(join(tmpdir(), "sandcastle-independent-"));
  t.after(() => {
    const path = realpathSync(temporary);
    assert.ok(path.startsWith(realpathSync(tmpdir()) + sep));
    assert.ok(basename(path).startsWith("sandcastle-independent-"));
    rmSync(path, { recursive: true, force: true });
  });
  const root = fileURLToPath(new URL("..", import.meta.url));
  const manifest = JSON.parse(readFileSync(join(root, "package.json")));
  const archive = resolve(
    root,
    manifest.devDependencies["@rebuild/sandcastle"].slice(5),
  );
  const provenance = JSON.parse(
    readFileSync(join(root, ".sandcastle/package-provenance.json")),
  );
  assert.equal(
    createHash("sha256").update(readFileSync(archive)).digest("hex"),
    provenance.sha256,
  );
  assert.match(provenance.revision, /^[a-f0-9]{40}$/);
  assert.match(provenance.image, /^sha256:[a-f0-9]{64}$/);
  writeFileSync(
    join(temporary, "package.json"),
    JSON.stringify({
      name: "independent-consumer",
      private: true,
      type: "module",
      dependencies: {
        "@rebuild/sandcastle": `file:${archive.replaceAll("\\", "/")}`,
      },
    }),
  );
  const args = [
    "install",
    "--offline",
    "--ignore-scripts",
    "--no-audit",
    "--no-fund",
  ];
  execFileSync(
    process.platform === "win32" ? "cmd.exe" : "npm",
    process.platform === "win32"
      ? ["/d", "/s", "/c", "npm.cmd " + args.join(" ")]
      : args,
    { cwd: temporary, encoding: "utf8", windowsHide: true, timeout: 60000 },
  );
  const installed = join(temporary, "node_modules/@rebuild/sandcastle");
  const inspect = (directory, prefix = "") => {
    for (const entry of readdirSync(directory, { withFileTypes: true })) {
      const name = prefix + entry.name;
      assert.doesNotMatch(
        name,
        /(?:^|\/)(?:\.sandcastle|codex-home|logs|worktrees|sessions?|auth\.json|credentials(?:\.json)?|\.npmrc|\.env(?:\.[^/]*)?)(?:\/|$)/,
      );
      const path = join(directory, entry.name);
      if (entry.isDirectory()) inspect(path, name + "/");
      else
        assert.doesNotMatch(
          readFileSync(path, "utf8"),
          /C:[\\/]FinalPolishedProjects|AAAiee\/GameFramework|prompt-A|fixture-token-A/,
        );
    }
  };
  inspect(installed);
  for (const name of ["workflow", "workflow-integrations", "workflow-runtime"])
    assert.ok(readFileSync(join(installed, `docs/${name}.md`)).length > 0);
  copyFileSync(
    new URL("./consumer-fixture.mjs", import.meta.url),
    join(temporary, "verify.mjs"),
  );
  const environment = { ...process.env };
  delete environment.NODE_TEST_CONTEXT;
  const output = execFileSync(process.execPath, ["verify.mjs"], {
    cwd: temporary,
    env: environment,
    encoding: "utf8",
    windowsHide: true,
    timeout: 30000,
  });
  assert.match(output, /PASS: independent packaged A then B/);
});
