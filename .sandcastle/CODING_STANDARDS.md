# Coding Standards

## Style

- The first-party projects use C++20. Do not introduce a newer language requirement without an explicit architecture decision.
- Preserve the naming, indentation, brace placement, include ordering, and file organization of the surrounding first-party code. The repository does not currently define a root `.clang-format`; avoid broad formatting churn.
- Keep changes narrowly scoped to the issue. Do not mix unrelated cleanup into an implementation commit.
- Treat `Engine/RBVendor/` and `Vendor/` as third-party code. Do not edit them unless the issue explicitly requires a vendor change.
- Keep generated Visual Studio files and build outputs out of commits (`*.sln`, `*.vcxproj*`, `.vs/`, and `Binaries/`).

## Testing

- Add or update a focused regression test when the affected first-party code has a viable test seam.
- This Linux sandbox cannot perform the authoritative project build. Never claim a successful Windows build from sandbox-only inspection.
- Before an issue is closed, generate the solution with `Scripts/Setup-Windows.bat` when needed and build `Rubber Engine.sln` for x64 using MSVC/MSBuild through Visual Studio 2022 or JetBrains Rider.
- Record feature-specific runtime or manual checks in the issue handoff.

## Architecture

- First-party engine code lives under `Engine/Source/`; editor application code lives under `Editor/src/`.
- Preserve the separation between the `Engine` static library and the `Editor` console application defined by the Premake build files.
- Read `GLOSSARY.md` and relevant records under `docs/adr/` when they exist. Flag conflicts rather than silently overriding established decisions.
- Prefer small interfaces and changes localized to the module that owns the behavior. Avoid new cross-module coupling unless the ticket or an ADR justifies it.
