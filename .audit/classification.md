# Existing checks, classified for dedupe

Not a coverage target. F01-F14 and issues #12-#25 appear so the new suite does not clone their oracles.

| name | source | asserts | needs | before glfwInit | class |
| --- | --- | --- | --- | --- | --- |
| camera-inverse | CameraInverseCheck.cpp, UnitTests.cpp, Regression.cpp | owned inverse snapshot, projection unchanged, product identity 1e-5 | none | yes | already-unit-and-still-in-Regression. Keep the UnitTests case. Do not write a third copy. |
| layer-partition | LayerPartitionCheck.cpp | ordinary/overlay order, absent pop is no-op, dtor deletes owned layers | none | yes | promote-to-new-unit with fresh doctest of LayerStack. Leave the selector on the host suite. Do not copy its require() list. |
| typed-events | EventChecks.cpp | typed identity, immediate dispatch, current/next queues | Logger | yes | promote-to-new-unit with fresh EventManager doctest. Do not copy EventChecks. |
| event-names | EventChecks.cpp | subscribe name uniqueness / lookup | Logger | yes | same as typed-events |
| font-atlas | FontAtlasCheck.cpp | named TTF load, exact string_view lookup, default font | ImGui context, font files | yes | keep-as-host. Survived hosted CI. Needs ImGui. Do not clone into doctest. |
| gl-buffers | GLBuffersCheck.cpp | GL buffer objects | Application, GL 4.5 | no | too-heavy-for-hosted-CI. WGL missing on windows-latest. |
| gl-boundaries | GLBoundariesCheck.cpp | GL state boundaries | Application, GL | no | too-heavy-for-hosted-CI |
| atlas-placeholders | AtlasPlaceholdersCheck.cpp | missing atlas slots, GL readback | GL 4.5, AssetManager | no | too-heavy-for-hosted-CI |
| scene-hooks | Regression.cpp checkSceneHooks | Tag/Transform/Sprite/Camera/Visibility add/remove, camera hook viewport | Application, GL, Logger | no | keep-as-host. New unit tests cover registry ops on a no-systems Scene. Do not copy hook oracles. |
| renderer-batches | RendererBatchesCheck.cpp | 10k quads, texture slots, GL draw counts | Application, GL | no | too-heavy-for-hosted-CI |
| entity-tags / editor-issue-16 | EntityTagsCheck.cpp, issue-16.ps1 | Inspector tag input, add Camera/Visibility, delete entity | Application, GL, ImGui | no | too-heavy-for-hosted-CI |
| editor-issue-12 | EditorIssue12Fixture.cpp, issue-12.ps1 | editor loop, framebuffer, tag edit, delete | Application, GL, ImGui | no | too-heavy-for-hosted-CI |
| editor-issue-15 | EditorIssue15Fixture.cpp, issue-15.ps1 | native Save As/Open, Unicode, overwrite, capacity | Application, GL, native dialog, private desktop | no | too-heavy-for-hosted-CI |
| viewport-pixels / editor-issue-17 | ViewportPixelsCheck.cpp, issue-17.ps1 | fractional/zero viewport, camera ratio | Application, GL, ImGui | no | too-heavy-for-hosted-CI |
| minimize-restore / editor-issue-18 | MinimizeRestoreCheck.cpp, issue-18.ps1 | native minimize, fixed update cadence | Application, native window | no | too-heavy-for-hosted-CI |
| unit-tests.ps1 | Tests/Windows/unit-tests.ps1 | build, run, deliberate CHECK(false) probe | MSBuild | n/a | host acceptance. CI runs UnitTests.exe, not the probe rebuild. |

F01 scene hooks, F02 camera inverse, F03 batches, F04 dialogs, F05 tags, F06 viewport, F07 minimize, plus later layer/event/font/GL selectors, are all in this table. New tests target module behavior, not those issue numbers.
