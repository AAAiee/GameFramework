# Editor UI fixtures

Prefer the [native workflow](native-validation.md) for bounded execution and
owned resource reuse. Regression creates and attaches a private Windows desktop
**before** GLFW/Application initialization. It never switches the input desktop
and fails closed if isolation fails. Real production dialogs remain automated
on that desktop: no manual clicks, mocked dialog fallback or input-desktop rerun.

The existing wrappers accept `-Configuration Debug|Release` and absolute
`-BinaryRoot`. From the repository root, after building an already prepared
worktree, a focused foreground invocation is:

```powershell
$binaryRoot = Join-Path $prepared 'Binaries/windows-x86_64/Debug'
pwsh.exe -NoProfile -File (Join-Path $prepared 'Tests/Windows/issue-15.ps1') `
  -Configuration Debug -BinaryRoot $binaryRoot
if ($LASTEXITCODE -ne 0) { throw 'Editor issue-15 fixture failed; retain its logs' }
```

Require the exact marker below as well as exit zero. The wrapper checks the
fixture exit and marker before printing its own marker. Each runs with the
Regression output directory as working directory and retains its
`issue-<number>.stdout.log` and `issue-<number>.stderr.log` there.
Timeout cleanup targets the invocation's child, not unrelated processes.
These direct wrappers do not establish the workflow adapter's JobObject
ownership or safe restoration guarantee.

| Wrapper | Genuine behavior asserted | Exact marker | Timeout |
| --- | --- | --- | --- |
| [issue-12.ps1](../Windows/issue-12.ps1) | Editor attachment/frame rendering, supported components, tag editing and deletion | `PASS: editor-issue-12` | 60 s |
| [issue-15.ps1](../Windows/issue-15.ps1) | New, Unicode Save As/Open, cancel, overwrite decline/confirm and capacity rejection | `PASS: editor-issue-15` | 240 s |
| [issue-16.ps1](../Windows/issue-16.ps1) | Actual hierarchy input for long/percent/Unicode tags, stable row IDs, selection/deletion, Inspector Camera/Visibility additions and Primary/visibility checkbox edits | `PASS: editor-issue-16` | 60 s |
| [issue-17.ps1](../Windows/issue-17.ps1) | Fractional/zero viewport, real framebuffer/camera events, New/Open and style restoration | `PASS: editor-issue-17` | 240 s |
| [issue-18.ps1](../Windows/issue-18.ps1) | Native minimize/restore, suspended fixed updates and resumed 1/120 s cadence without backlog | `PASS: editor-issue-18` | 60 s |

Fixtures compile production EditorLayer, hierarchy, GL and ImGui behavior.
#12 mutates the real scene component API while production panels render.
These observations cover those fixtures; they do not automate every standalone
Editor.exe menu or establish ordinary human editing results. Leave unperformed
human observations explicitly unrun in their own report.

Final acceptance runs all five wrappers in Debug **and** Release, in addition
to full Regression and standalone selectors. Preserve current outcome evidence
in host reports; [original dialog attempts](../HISTORY.md#native-dialog-automation-and-desktop-isolation)
remain historical checkpoints.
