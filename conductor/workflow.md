# Workflow

## TDD Policy

**Strict.** Tests must be written before or alongside implementation — never after. A task is not complete until its tests pass. PRs that add functionality without corresponding tests will not be merged.

- New types and utilities require unit tests in `tests/`
- Test files mirror the source structure (e.g. `TestVec.cpp` for `Vec.hpp`)
- Tests are registered in `tests/CMakeLists.txt`

## Commit Strategy

**Conventional Commits.** All commits must follow the format:

```
<type>(<scope>): <short description>

[optional body]

[optional footer]
```

Common types:
- `feat` — new feature or API addition
- `fix` — bug fix
- `refactor` — code restructure without behavior change
- `test` — adding or updating tests
- `docs` — documentation only
- `chore` — build system, CI, tooling changes
- `perf` — performance improvement

Example: `feat(String): add split support for multi-character delimiters`

## Code Review

**Required for all changes via GitHub Pull Request.**

- All changes must go through a PR — no direct pushes to `develop` or `main`
- At least one approval required before merge
- CI must pass before merge

## Verification Checkpoints

**Manual verification required after each phase completion.**

Before marking a phase complete:
1. All tests in the phase pass (`ctest` or equivalent)
2. No regressions in existing tests
3. Build succeeds in both Debug and Release configurations
4. Reviewer has approved the phase's PR

## Task Lifecycle

```
planned → in_progress → review → verified → complete
```

- `planned` — task defined, not started
- `in_progress` — actively being worked
- `review` — PR open, awaiting review
- `verified` — phase checkpoint passed
- `complete` — merged and closed
