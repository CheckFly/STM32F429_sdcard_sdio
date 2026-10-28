---
name: git-workflow
description: Use Git safely for the F429_sdcard_sdio_ecran STM32 project, including initialization, commits, branches, version tags 0.0x, changelog updates, and LVGL/FatFs submodules. Use for versioning, history, releases, and collaboration.
license: MIT
compatibility: opencode
metadata:
  project: F429_sdcard_sdio_ecran
  versioning: 0.0x
---

# Git workflow for the STM32 project

## What I do

- Initialize and inspect the repository.
- Create focused commits with a clear convention.
- Create and merge branches for features and fixes.
- Tag validated versions (`v0.01`, `v0.02`, …).
- Manage `.gitignore`, releases, and submodules.
- Keep `docs/HISTORY.md` aligned with Git history.

## When to use me

Use me before and after each feature, test, build, or release. Never commit build artifacts or secrets.

## Repository state

This directory may not be a Git repository yet. Initialize it once:

```powershell
git init
git branch -M main
git add -A
git commit -m "chore: initial STM32 project snapshot"
```

Check the state:

```powershell
git status --short
git log --oneline --decorate -10
```

## Commit convention

```text
<type>(<scope>): <short imperative summary>
```

Types:

| Type | Use for |
|------|---------|
| `feat` | New feature (LVGL screen, FatFs mount, SD detection) |
| `fix` | Bug fix (pin mode, UART, timing) |
| `build` | Makefile, CI, toolchain |
| `docs` | Documentation, history, steps |
| `test` | Test scripts and results |
| `refactor` | Code restructuring without behavior change |
| `chore` | Repository maintenance |

Examples:

```text
feat(sd): emit SD:PRESENT and SD:REMOVED on COM39
fix(gpio): configure PA10 as input with pull-up
build(ci): add GitHub Actions firmware build
docs(sd): document the 4 GB + remaining-space layout
test(sd): add COM39 insert/remove smoke test
```

## Versioning and tags

- `VERSION` and `config/project.json` carry the current version.
- Validated versions are tagged:

```powershell
git tag -a v0.02 -m "Validated SD detection and FatFs mount"
git push origin main --tags
```

- Tag only after:
  1. build succeeds,
  2. flash succeeds,
  3. the smoke test passes,
  4. `docs/HISTORY.md` and `docs/TESTS.md` are updated.

## Branch strategy

```text
main                  stable, validated versions
feature/<name>        new feature
fix/<name>            bug fix
test/<name>           test-only work
docs/<name>           documentation-only work
```

Create and merge:

```powershell
git switch -c feature/lvgl-port
# work, commit
git switch main
git merge --no-ff feature/lvgl-port
```

## Daily commands

```powershell
git status
git diff
git add <files>
git commit -m "feat(display): add LVGL flush callback"
git pull --rebase
git push origin main
```

## Submodules for LVGL and FatFs

```powershell
git submodule add https://github.com/lvgl/lvgl.git Middlewares/Third_Party/LVGL
git submodule update --init --recursive
git add .gitmodules Middlewares/Third_Party/LVGL
git commit -m "build(middleware): add LVGL submodule"
```

When cloning:

```powershell
git clone --recurse-submodules <url>
```

## .gitignore rules

Ignored:

- `build/`, `Debug/`, `Release/`
- `*.elf`, `*.bin`, `*.hex`, `*.map`, `*.list`
- `*.o`, `*.d`, `*.su`, `*.cyclo`
- `.settings/`, `.metadata/`, `*.launch`
- Python caches, logs, `.env`

Never ignore:

- `Core/`, `Drivers/`, `Middlewares/`
- `.opencode/skills/`
- `docs/`, `config/`, `tests/`, `scripts/`
- `.github/workflows/`

## Safety rules

- Never use `git push --force` on `main`.
- Never commit secrets, tokens, or `.env`.
- Never commit a `.elf` or `.bin` unless a release explicitly requires it.
- Verify `git status` before `git add -A`.
- If a generated file is already tracked, remove it from the index without deleting it:

```powershell
git rm --cached path/to/generated/file
```

## Changelog and release checklist

- [ ] `VERSION` and `config/project.json` updated.
- [ ] `docs/HISTORY.md` updated.
- [ ] `docs/STEPS.md` updated.
- [ ] `docs/TESTS.md` updated with the test result.
- [ ] Build and flash succeeded.
- [ ] Git tag created (`v0.0x`).
- [ ] Tag pushed.
