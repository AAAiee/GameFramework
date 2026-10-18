import { checkBaseline } from "./check-baseline.mjs";
import { inspectQueue } from "./queue.mjs";
import { project } from "./project.mjs";

const args = process.argv.slice(2);
if (
  args.length > 1 ||
  (args.length === 1 && !["--check", "--queue"].includes(args[0]!))
)
  throw new Error(
    "Supported commands: --check or --queue. Ticket execution is not enabled in this increment.",
  );
console.log(
  JSON.stringify(
    args[0] === "--check" ? await checkBaseline() : await inspectQueue(project),
    null,
    2,
  ),
);
