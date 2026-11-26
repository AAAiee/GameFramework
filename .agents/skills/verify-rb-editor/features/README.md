# RB Editor verification map

This directory is the maintained source for verifying the user-facing behavior of RB Editor. Read the index before driving the app, then use the matching feature file as the recipe.

## Baseline preconditions

- Windows x64 display host with an OpenGL 4.5 driver.
- Repo root contains `Build.lua` and `Editor\src\Editor.cpp`.
- `rb-verify.ps1 build` has produced Debug `Editor.exe` and `Regression.exe` with copied `assets\`.
- `rb-verify.ps1 doctor` prints `OK` for `editor`, `regression`, and both asset trees.
- Never drive an `Editor.exe` whose PID is missing from this run's `Binaries\Verify\<run-id>\instance\state.json`.
- One `drive` at a time.

## Driving conventions

- Start every recipe from the baseline state unless its preconditions say otherwise.
- Run unattended journeys through `rb-verify.ps1 drive`.
- Treat every command as literal. Keep feature ids and flags unchanged.
- Use the widget names in `Tests\src\UiCatalog.h`. Feature files name the ones each journey reaches.
- Restore nothing in the build tree. Isolated Editor copies die in `cleanup`. Regression work trees copied into `evidence\` stay there.

## Proof and skip reporting

- Capture the user action and the resulting state, not only the final screen.
- Regression proof is `stdout.log`, `stderr.log`, `summary.txt`, and any copied `issue-*-work-*` directory under `Binaries\Verify\<run-id>\evidence\`.
- Live Editor proof adds `editor.png` from `capture`, with the window title `Editor` visible.
- Mutation proof includes a second read of stored state. A `.rubber` save is proven by opening that file and matching Tag, Transform, Sprite color, Camera, and Visibility fields.
- Record the feature id and entry point used with every artifact.
- A catalogued Editor control is driven by `drive editor-ui`, except the `File` / `Exit` item, which is `drive ui-exit`. Escape is `drive ui-escape`. Do not report `verified-unreachable` for a catalog id.
- Hosted CI runs `Tests/Windows/scan-ui-catalog.ps1` and `UnitTests`. Neither runs live Editor clicks.
- Do not report a skipped entry point as verified through a different path.

## Feature entry contract

Each feature file starts with an H1 title and one paragraph describing the user-visible behavior. It then uses exactly four H2 sections in this order.

1. `Sub-features` lists short IDs with one line for each behavior.
2. `How to get to it (user POV)` lists every user entry point.
3. `Driving it with rb-verify` starts with `Preconditions:` and uses labeled bullets that pair each user action with an exact command and observable result.
4. `Gotchas` lists traps that can waste or invalidate a verification run.

Keep implementation details out of the map. Name only user paths, stable handles, required state, commands, and observable proof.

## Features

- [Scene hierarchy](./scene-hierarchy.md) covers entity create, select, rename, deselect, and delete in Scene Hierarchy.
- [Inspector](./inspector.md) covers Tag, Add Component, Camera Primary, Visibility, Transform, and Sprite color in Property Channel.
- [Scene files](./scene-files.md) covers File New, Open, Save As, cancel, overwrite, and Unicode `.rubber` round trips.
- [Viewport](./viewport.md) covers ViewPort framebuffer resize, camera aspect, and New/Open at usable pixel sizes.
- [Window lifecycle](./window-lifecycle.md) covers native minimize and restore of the Editor window.
