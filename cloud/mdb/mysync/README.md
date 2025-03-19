## Mysync

## Tests

Run all tests:
`cd tests ; make test`

In order to run particular test (e.g. tests/features/zk_failure.feature):
`cd tests ; GODOG_FEATURE="zk_failure" make test`


### MacOS
Cross-compilation from MacOS to linux:

`ya package pkg.json --target-platform=default-linux-x86_64`
