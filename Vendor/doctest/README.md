# doctest v2.4.12

Unmodified upstream single-header distribution and MIT license, pinned to commit
`1da23a3e8119ec5cce4f9388e91b065e20bf06f5` (v2.4.12).

| File | Exact source | SHA-256 (upstream bytes) |
| --- | --- | --- |
| `doctest.h` | https://raw.githubusercontent.com/doctest/doctest/1da23a3e8119ec5cce4f9388e91b065e20bf06f5/doctest/doctest.h | `94029a7d32da24a56249658147dbd2b33ff0b9ed665295cbbaf19aafff5b0ced` |
| `LICENSE.txt` | https://raw.githubusercontent.com/doctest/doctest/1da23a3e8119ec5cce4f9388e91b065e20bf06f5/LICENSE.txt | `0fe0b331fa1513dcce8604ff1fa925f32d1cea17d8aeb1c2471fad40d291adc5` |

Ordinary generation, builds and test runs use these checked-in files. No package
manager, download step, submodule or framework library is required for doctest.
GLM remains the repository's existing pinned dependency.

## Intentional updates

1. Review an upstream release and resolve its tag to an exact commit. Review its
   changes and license before updating.
2. Fetch only `doctest/doctest.h` and `LICENSE.txt` from that commit into a staging
   directory. Preserve the original bytes; do not regenerate or edit the header.
3. Calculate SHA-256 for both files (`Get-FileHash -Algorithm SHA256` in PowerShell
   or `sha256sum`), review the staged diff, and replace both vendored files.
   Update the version, commit, exact URLs and checksums in this document together.
4. Generate VS2022 projects and build/run x64 Debug and Release UnitTests using
   [the local commands](../../Tests/README.md#headless-unittests-issue-29).
   Verify console/filter/JUnit results and nonzero exits from a temporary failing
   assertion in both configurations. Remove the probe and rebuild/run again.
   Retain the existing Regression and private-desktop UI acceptance on the host.
5. Submit the vendor update and evidence for review as an intentional dependency
   update. Never add automatic downloads to ordinary validation.
