# Evidence freshness

The combined #38/#39 integration package selects committed fork revision
`f8248556c6c2fbcfa504ed859479ab8566fd2f70`. The canonical offline Linux build
produced archive SHA-256
`7d4708dd9325702165c85cf431a209cea7d92acf9bd5d52148f99fb042eb7512`.
The tracked [package-provenance.json](package-provenance.json), vendor archive,
dependency and lockfile carry the same identity; original registry SDK stays
exact `0.12.0`. One canonical build was performed for this increment, with
independent final-artifact reproduction still required by #43.

`runEditorTask({ evidence, ...options })` enables project-owned identity discovery.
The original fail-fast consumer stays compatible; once an identity-aware
checkpoint exists, current discovery is mandatory on resume.

`createEditorEvidence` from `./evidence.mjs` is the Editor identity adapter. Supply
four async callbacks taking the public workflow context:

- `validationPolicy`: current final/diagnostic plan, selectors and validator bytes.
- `dependencies`: current observed installed/prepared native pins, beyond Git's
  declared pins. Native acceptance must verify these observations.
- `environment`: current relevant MSBuild/compiler/SDK and graphics driver identity.
  This must describe the actual validation host. The Docker builder image alone
  is insufficient. Unavailable discovery stops instead of returning a sentinel.
- `reviewPolicy`: independent reviewer policy/model/context revision.

The adapter also binds the actual candidate lock and gitlinks, current runner
lock/provenance and installed workflow bytes, task adapter content, and actual
Node/platform/architecture. The task adapter binds its actual review prompt;
workflow core binds candidate, task criteria and validation identity automatically.
Callbacks must refresh observations across resume. They must not return saved
identities or volatile report outcomes. Native project discovery is supplied by
the native validation adapter; this generic slice does not claim to discover or
validate MSBuild or graphics itself.

Matching passing reports can be reused. Policy, dependency or environment changes
revalidate the same code and require fresh independent approval; review-only
changes retain matching host validation and repeat review. Runtime compatibility
still identifies resource-lifecycle compatibility. Do not use it as a changing
policy/environment fingerprint: evidence freshness is the separate input above.
Task scope changes remain an explicit checkpoint safety refusal in Editor.

Identity-aware local handoffs use immutable `handoff-<digest>.json` records bound
to complete task/candidate/validation/review. Old receipts remain available when
same-SHA code is revalidated. Reconciliation reads the exact expected record and
compares its evidence/approval; candidate equality alone is insufficient. Legacy
non-identity consumers retain `handoff.json`. No path creates a PR or changes an
issue. Current validation timing rows contain only checks recorded by the current
validation invocation; historical same-SHA rows remain in the measurement report.

Host reports and checkpoints own current outcomes. No code commit is required to
contain its own SHA. These deterministic fixtures exercise reload and identity
gates, not live provider or native acceptance.
