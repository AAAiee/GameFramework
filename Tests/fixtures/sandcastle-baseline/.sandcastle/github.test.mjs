import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, writeFileSync, readFileSync, realpathSync, rmSync } from "node:fs";
import { join, sep, basename } from "node:path";
import { tmpdir } from "node:os";
import { github, command } from "./host.mjs";

function fixture(t, failures = {}) {
  const directory = mkdtempSync(join(tmpdir(), "editor-github-test-"));
  t.after(() => {
    const resolved = realpathSync(directory);
    if (!resolved.startsWith(realpathSync(tmpdir()) + sep) || !basename(resolved).startsWith("editor-github-test-")) {
      throw new Error("Unexpected test directory.");
    }
    rmSync(resolved, { recursive: true, force: true });
  });
  const report = join(directory, "results.md");
  writeFileSync(report, "Automated validation: PASS\n");
  const active = { number: 12, sha: "reviewed", validation: { report }, review: { criteria: [] } };
  const comments = [];
  const calls = [];
  const delays = [];
  let postCount = 0;
  let getCount = 0;
  const execute = (executable, args, cwd, options) => {
    const post = args.includes("POST");
    calls.push({ args, post, timeout: options.timeout });
    if (!post) {
      getCount++;
      const failure = failures.get?.(getCount);
      if (failure) throw failure;
      return JSON.stringify([comments]);
    }
    postCount++;
    const body = JSON.parse(readFileSync(args[args.indexOf("--input") + 1], "utf8")).body;
    const failure = failures.post?.(postCount);
    const committed = typeof failures.committed === "function" ? failures.committed(postCount) : failures.committed;
    if (!failure || committed) comments.push({ id: postCount, body });
    if (failure) throw failure;
    return JSON.stringify(comments.at(-1));
  };
  const gh = github(directory, "owner/repo", directory, {
    env: {}, command: execute, sleep: (delay) => delays.push(delay), warn: () => {},
  });
  return { gh, active, comments, calls, delays };
}

const timeout = () => Object.assign(new Error("spawnSync gh.exe ETIMEDOUT"), { code: "ETIMEDOUT" });

test("Windows proxy connection resets retry reads without rerunning acceptance", (t) => {
  const f = fixture(t, { get: (count) => count === 1
    ? new Error('read tcp 127.0.0.1:59789->127.0.0.1:18081: wsarecv: An existing connection was forcibly closed by the remote host.') : null });
  f.gh.publish(f.active);
  assert.equal(f.comments.length, 1);
  assert.deepEqual(f.delays, [2000]);
});

test("timed-out comment already committed is reconciled without another POST", (t) => {
  const f = fixture(t, { post: timeout, committed: true });
  f.gh.publish(f.active);
  assert.equal(f.comments.length, 1);
  assert.equal(f.calls.filter((call) => call.post).length, 1);
  assert.ok(f.calls.filter((call) => !call.post).length >= 2);
});

test("timed-out uncommitted comment is retried after checking its marker", (t) => {
  const f = fixture(t, { post: (count) => count === 1 ? timeout() : null });
  f.gh.publish(f.active);
  assert.equal(f.comments.length, 1);
  const firstPost = f.calls.findIndex((call) => call.post);
  assert.equal(f.calls[firstPost + 1].post, false);
  assert.equal(f.calls.filter((call) => call.post).length, 2);
  assert.ok(f.delays.length);
});

test("read timeouts retry with a VPN-friendly timeout and bounded backoff", (t) => {
  const f = fixture(t, { get: (count) => count < 3 ? timeout() : null });
  f.gh.publish(f.active);
  assert.equal(f.comments.length, 1);
  assert.ok(f.calls.every((call) => call.timeout === 120_000));
  assert.deepEqual(f.delays, [2000, 4000]);
});

test("a permanent authorization failure is not retried", (t) => {
  const f = fixture(t, { post: () => new Error("HTTP 403: Resource not accessible") });
  assert.throws(() => f.gh.publish(f.active), /HTTP 403/);
  assert.equal(f.calls.filter((call) => call.post).length, 1);
  assert.equal(f.delays.length, 0);
});

test("persistent timeout is bounded and cannot be mistaken for publication", (t) => {
  const f = fixture(t, { post: timeout });
  assert.throws(() => f.gh.publish(f.active), /ETIMEDOUT/);
  assert.equal(f.calls.filter((call) => call.post).length, 3);
  assert.equal(f.comments.length, 0);
});

test("existing commit marker makes resuming publication idempotent", (t) => {
  const f = fixture(t);
  f.gh.publish(f.active);
  f.gh.publish(f.active);
  assert.equal(f.comments.length, 1);
  assert.equal(f.calls.filter((call) => call.post).length, 1);
});

test("a lost response on the last POST is still reconciled", (t) => {
  const f = fixture(t, { post: timeout, committed: (count) => count === 3 });
  f.gh.publish(f.active);
  assert.equal(f.comments.length, 1);
  assert.equal(f.calls.filter((call) => call.post).length, 3);
});

test("uncertain POST followed by unavailable reads stops rather than blindly resending", (t) => {
  let unavailable = true;
  const f = fixture(t, { post: timeout, committed: true,
    get: (count) => unavailable && count >= 2 ? timeout() : null });
  assert.throws(() => f.gh.publish(f.active), /ETIMEDOUT/);
  assert.equal(f.calls.filter((call) => call.post).length, 1);
  unavailable = false;
  f.gh.publish(f.active);
  assert.equal(f.calls.filter((call) => call.post).length, 1);
});

test("the real CLI wrapper preserves the operating-system timeout code", () => {
  assert.throws(() => command(process.execPath, ["-e", "setTimeout(() => {}, 2000)"], process.cwd(), { timeout: 20 }),
    (error) => error.code === "ETIMEDOUT" && error.cause.code === "ETIMEDOUT");
});
