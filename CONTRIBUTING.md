# Contributing

Keep changes focused on one behavior. Describe the problem, the resulting
behavior and the validation in English. Preserve existing runtime translations
and compatibility with both supported board revisions.

## Formatting

Use the repository `.clang-format` for first-party C and C++ code (LLVM base,
two-space indentation, 100-column limit, existing include order). Clang-format
18 is the reference version. Format the functions or lines you change; avoid
repository-wide reformatting mixed with functional changes. Existing files
contain older styles, so a whole-file format check is not a merge requirement.

For a new first-party C++ file:

```sh
clang-format --style=file -i path/to/new_file.cpp
clang-format --style=file --dry-run --Werror path/to/new_file.cpp
```

Use UTF-8 and LF line endings. Match surrounding style in Python, JavaScript,
HTML and workflow files. Do not reformat vendored libraries or generated output.
For generated locales and compressed web assets, edit their source catalogs or
assets and run the corresponding generator; CTest checks generated consistency.

## Validation and review

Follow the [test strategy and commands](test/README.md). Add a regression test
when it protects a bug fix or meaningful new behavior. Prefer production modules
with small boundary adapters over a second implementation inside the test.

Run `git diff --check` and inspect the diff for unrelated changes. Firmware
changes must pass host tests, cppcheck and all six SDK builds in CI before
merging. Document hardware observations separately and distinguish them from
host simulation and compile checks. Documentation-only changes need relevant
link/content checks rather than a new firmware release.

Keep public API and stored-data compatibility explicit. Never include private
credentials, device backups or personal reading data in commits or issue logs.
