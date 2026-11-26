# Scene hierarchy

Scene Hierarchy lists every entity in the active scene. A user creates an empty entity, selects a row, edits its tag in the inspector, deselects by clicking blank space, and deletes the entity from the row context menu.

## Sub-features

- `hierarchy-create` creates an entity named `Empty Entity` from the blank-space context menu.
- `hierarchy-select` selects a row by left-click and shows it in Property Channel.
- `hierarchy-rename` edits the Tag field and keeps the same row identity.
- `hierarchy-deselect` clears selection when the user left-clicks blank hierarchy space.
- `hierarchy-delete` removes the entity through `Delete Entity` and clears selection.

## How to get to it (user POV)

- Right-click blank space in the `Scene Hierarchy` window and choose `Create Entity`.
- Left-click an entity row to select it.
- Edit the Tag input in `Property Channel`.
- Left-click blank space in `Scene Hierarchy` to deselect.
- Right-click a row and choose `Delete Entity`.

## Driving it with rb-verify

Preconditions:

- `rb-verify.ps1 doctor` reports `OK regression=` for Debug.
- No other `drive` holds `Binaries\Verify\.drive.lock`.

- **Create Entity and Delete Entity.** Drive the blank-space context menu and the row context menu on the production docked Scene Hierarchy. Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-hierarchy`. Exit code `0`. `evidence\summary.txt` contains `PASS: ui-hierarchy`, `OBSERVED: Create Entity menu added Empty Entity`, and `OBSERVED: Delete Entity removed Empty Entity`.
- **Select, rename, deselect, delete.** Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive scene-hierarchy`. Exit code `0`. `evidence\summary.txt` contains `PASS: editor-issue-16` and `OBSERVED: renamed row retained expansion and selection; blank-space deselection, reselection and Delete Entity menu passed`.
- **Literal tags.** The same drive, aliased `entity-tags`, types empty, `%`, `%s`, `%n`, UTF-8, 255-byte, 256-byte, and 8192-byte tags through the production Tag input. `evidence\summary.txt` contains `OBSERVED: real hierarchy input retained empty, percent, UTF-8, 255/256 and 8192+ byte tags; labels and IDs checked`.
- **Proof.** Copy `Binaries\Verify\<run-id>\evidence\scene-hierarchy\stdout.log` and `summary.txt`. Both files identify `editor-issue-16` and the two `OBSERVED:` lines above.

## Gotchas

- `drive scene-hierarchy` spawns its starting row through `Scene::createEntity("initial")`. Row select, tag editing, and `Delete Entity` go through `EditorDriver` clicks on labeled items. The row arrow is a `clickPoint` on the `ItemProbe` row rect, and blank-space deselect is a `clickPoint` on the hierarchy window's inner rect. The blank-space `Create Entity` menu is `drive ui-hierarchy`. Do not treat a spawned row as the Create Entity menu.
- Row identity is the ImGui id from the entity handle, not the tag string. After a rename, assert the same row can be reselected by click.
- An empty tag renders no label text. Assert the component bytes, not a visible name.
- Visible Unicode glyphs depend on OpenSans coverage. The fixture still requires exact UTF-8 bytes in the tag.
- Do not send clicks into a live `Editor.exe` by screen coordinate. That is not a listed driver.
