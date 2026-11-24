# Viewport

ViewPort shows the scene framebuffer. Resizing the panel resizes the framebuffer and updates cameras that do not use a fixed aspect ratio. New and Open must start from usable pixel sizes.

## Sub-features

- `viewport-resize` truncates a fractional panel size to integer pixels, keeps attachment ids when the integer size is unchanged, and publishes one resize event.
- `viewport-invalid` ignores zero, negative, NaN, and infinite panel sizes and keeps the last usable target.
- `viewport-camera` delivers the new aspect ratio to cameras after two event-dispatch frames.
- `viewport-new-open` runs File New and Open at fractional, zero, and subpixel panel sizes and still constructs a finite camera projection.
- `viewport-stats` shows DrawCount, QuadCount, IndexCount, VertexCount, Frame Time, and Current FPS in `Setting`.

## How to get to it (user POV)

- Dock or resize the `ViewPort` window.
- Create or open a scene while the panel is collapsed or fractional.
- Read render stats in the `Setting` window.

## Driving it with rb-verify

Preconditions:

- `rb-verify.ps1 doctor` reports `OK regression=` for Debug.
- No other `drive` holds `Binaries\Verify\.drive.lock`.

- **Live splitter drag and Setting stats.** Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive ui-viewport`. Exit code `0`. `evidence\summary.txt` contains `PASS: ui-viewport`, `OBSERVED: ViewPort splitter drag changed panel size`, and `OBSERVED: Setting window submitted DrawCount stats`.
- **Pixel sizes and camera delivery.** Drive production EditorLayer, framebuffer attachments, and WindowSystem. Run `pwsh -NoProfile -File .agents/skills/verify-rb-editor/scripts/rb-verify.ps1 drive viewport`. Exit code `0`. `evidence\summary.txt` contains `PASS: editor-issue-17` plus `OBSERVED: stable 800.5x600.5 panel retained framebuffer and attachments at 800x600`, `OBSERVED: zero, negative and subpixel-to-zero panels preserved usable framebuffer and camera`, and `OBSERVED: changed pixels reached framebuffer, events and cameras; dockspace restored style`.
- **New/Open at awkward sizes.** The same drive opens a serialized camera through the production Open dialog at fractional and zero extents. `stdout.log` records those `OBSERVED:` dialog lines.
- **Proof.** Keep `evidence\stdout.log` and `summary.txt`. A green `Setting` panel screenshot without those markers is not viewport proof.

## Gotchas

- `drive viewport` still injects panel sizes at the layout/update boundary for pixel-quantization cases. The live dock splitter is `drive ui-viewport`.
- EventManager promotes a queued resize on the first dispatch and delivers it on the second. Checking the camera on the same frame as the resize fails.
- Unbind leaves the GL viewport at the framebuffer size. Do not assume the previous binding is restored.
- Resize of the framebuffer recreates attachments. Color attachment ids are not stable across a real resize.
- `viewport-stats` is asserted by `drive ui-viewport` through the live `Setting` window draw list. A `launch-editor` screenshot that shows `DrawCount:` is optional visual evidence only.
- Independent simultaneous scenes are unsupported. WindowSystem uses one shared `Viewport Resize` name.
