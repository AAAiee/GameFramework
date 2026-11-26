# Inspector

Property Channel edits the selected entity. A user changes the tag, adds Camera, Sprite Renderer, Transform, or Visibility, toggles camera Primary and visibility, and removes a component from the `...` menu.

## Sub-features

- `inspector-tag` edits the `##Tag` input for the selected entity.
- `inspector-add-camera` adds a Camera from `Add Component`.
- `inspector-add-visibility` adds Visibility from `Add Component`.
- `inspector-primary` toggles the Camera `Primary` checkbox.
- `inspector-visible` toggles the Visibility `isVisible` checkbox.
- `inspector-add-transform` adds Transform from `Add Component`.
- `inspector-add-sprite` adds Sprite Renderer from `Add Component`.
- `inspector-remove` removes a removable component through `Remove Component`.

## How to get to it (user POV)

- Select an entity in `Scene Hierarchy` so `Property Channel` shows its components.
- Type in the Tag input.
- Choose `Add Component`, then `Camera`, `Sprite Renderer`, `Transform`, or `Visibility`.
- Toggle `Primary`, `IsFixedRatio`, or `isVisible`.
- Open a component's `...` button and choose `Remove Component`. Tag cannot be removed from this menu.

## Driving it with rb-verify

Preconditions:

- `rb-verify.ps1 doctor` reports `OK regression=` for Debug.
- No other `drive` holds `Binaries\Verify\.drive.lock`.

- **Add Camera and toggle Primary.** Drive the production `Add Component` popup and `Primary` checkbox. Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive inspector`. Exit code `0`. `evidence\summary.txt` contains `PASS: editor-issue-16` and `OBSERVED: Inspector menus added Camera and Visibility; Primary and visibility checkboxes edited components through production widgets`.
- **Add Visibility and toggle isVisible.** The same drive clicks `Visibility` in the popup, asserts `isVisible` starts true, then toggles hide and show through the production checkbox.
- **Add Transform, Sprite Renderer, Camera, Visibility, and Remove Component.** Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-inspector`. Exit code `0`. `evidence\summary.txt` contains `PASS: ui-inspector`, `OBSERVED: Add Component / Sprite Renderer reached the registry`, `OBSERVED: Color Picker opened the hue-bar popup`, `OBSERVED: ColorEdit4 ##X drag edited Sprite color.r`, `OBSERVED: Add Component / Transform reached the registry`, `OBSERVED: Transform X, Y, Z reset buttons and drags edited the component`, `OBSERVED: Remove Component removed Sprite Renderer`, `OBSERVED: Add Component / Camera reached the registry`, `OBSERVED: Camera Primary, IsFixedRatio, and Size edited`, `OBSERVED: Projection switched to Perspective`, `OBSERVED: FOVY, NearClip, and FarClip drags edited the perspective camera`, `OBSERVED: Projection switched to Orthographic`, `OBSERVED: Add Component / Visibility reached the registry`, and `OBSERVED: isVisible checkbox toggled off`.
- **Proof.** Keep `evidence\stdout.log` and `summary.txt` for this run id. The `OBSERVED:` inspector line is required. A registry mutation performed outside Property Channel is not inspector proof.

## Gotchas

- `drive inspector` still uses selector `editor-issue-16` for a shorter Camera/Visibility path. The full inspector catalog, including Projection, Size, FOVY, Color Picker, and Remove Component, is `drive ui-inspector`. Do not mark those verified from `entity.addComponent` in another fixture.
- Add Component menu order in source is Camera, Sprite Renderer, Transform, Visibility. `drive inspector` opens only the Camera and Visibility rows. `drive ui-inspector` clicks each component by label.
- Choosing an already-present component closes the popup and does not add a second copy.
- Rotation in the Transform editor is shown in degrees and stored in radians.
- `EditorSmoke` adding a Transform through the component API is not this feature.
