# refactor

Refactor is tool for automating common tasks during global refactoring of go code in Arcadia.

## yolint

`yolint` subcommand displays linter errors from CI check. `check_id` is obtained from URL of CI page.

For example, if `See in CI` leads to url `https://ci.yandex-team.ru/check/24i6f/?test_type=&status=FAILED&only_important_diffs=1`,
check_id is equal to `24i6f`.

```bash
refactor yolint --check-id 2zfav | less # see full linter errors
refactor yolint --check-id 2zfav --stat # aggregate error statistics by top level
refactor yolint --check-id 2zfav --stat --depth 99 # see errors by package
refactor yolint --check-id 2zfav --migration # generate package list for migration.yml
```