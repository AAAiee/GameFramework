import { checkBaseline } from "./check-baseline.mjs";

if (process.argv.length > 2) {
  throw new Error(
    "This baseline only checks the installed SDK; ticket execution is paused.",
  );
}

console.log(JSON.stringify(await checkBaseline(), null, 2));
console.log(
  "Original SDK baseline verified. Automated ticket execution is paused.",
);
