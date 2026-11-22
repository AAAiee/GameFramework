# Experiment log

Run: GitHub Actions [45023232349](https://github.com/AAAiee/GameFramework/actions/runs/45023232349) on `windows-latest`, commit `494c32c`. Artifacts in `.audit/hosted-ci-probe/`.

## UnitTests (no window)

Debug and Release MSBuild of `Tests/UnitTests.vcxproj` exited 0. `UnitTests.exe --no-colors --no-breaks` exited 0 in both configurations. doctest reported 1 case, 1 assertion, SUCCESS. Measured.

Lap54 cannot compile locally. clang++ has no C++ standard library headers. vswhere reports no MSVC product. Inconclusive here, hosted run is the evidence.

## Regression selectors on the same runner

`Tests/Regression.vcxproj` Debug rebuild exited 0. Selectors:

| selector | exit | marker | note |
| --- | --- | --- | --- |
| camera-inverse | 0 | PASS | before glfwInit |
| layer-partition | 0 | PASS | before glfwInit |
| typed-events | 0 | PASS | before glfwInit |
| event-names | 0 | PASS | before glfwInit |
| font-atlas | 0 | PASS | ImGui CPU atlas, before glfwInit |
| gl-buffers | -2147483645 | none | `WGL: The driver does not appear to support OpenGL` then `Context Window Handler is null!` |

Anything that constructs `Application` or a GLFW OpenGL 4.5 context dies on hosted windows-latest. Measured.

## CLI without ImGui

`SceneSerializer::serialize/deserialize` already take a path. `EditorLayer::saveScene/openScene` only add `fileDialog`. Extracting `saveSceneTo`/`openSceneFrom` is two methods and two wrappers, about 20 lines. `newScene` and `Scene::createEntity` do not call ImGui. `Scene::create` still constructs `RendererSystem`, which creates a GL framebuffer and needs `Application`. Command-layer Save/Load without clicking panels is real. It is not a hosted-CI path if it calls `Scene::create()`.

## Recommendation (from these runs)

- Hosted CI runs full UnitTests Debug+Release. It does not run Application/GL smokes.
- Pre-GL Regression selectors survived. They are Fxx oracles. Do not copy them into doctest. Do not keep them as the new smoke.
- Engine and editor smokes that construct Application stay on a display/GL host.
- Ask the owner whether to add a self-hosted GPU runner for those smokes. Hosted windows-latest will not grow an OpenGL 4.5 driver.
- Do not add an ImGui robot or a software rasterizer.
