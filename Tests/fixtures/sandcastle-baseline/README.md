# Inactive pre-extraction recovery copy

These 16 files restore `.scratch/issue-28-baseline` from
supplied GameFramework commit `edb928f0dc2c17f200214b819bb632f14acaba0d`.
`manifest.json` lists the original 15 file checksums and a historical test result.
Git normalized six source blobs to LF; their original CRLF bytes were restored
using the manifest checksums. No other source content was changed.
This README was added by issue #32 and is not part of that original snapshot.

Run `npm run test:sandcastle-baseline` from the repository root to verify the
checksums and rerun all original 32 tests. Do not execute this fixture's main
entrypoint or follow its historical WORKFLOW.md deployment instructions: those
include obsolete automatic merge/enqueue/closure policies. It is recovery
provenance, not an active runner or a delivery into Editor. See
[the preservation record](../../../docs/sandcastle-baseline.md).
