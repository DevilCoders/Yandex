## MDB Yandex.Tank-Pandora benchmarks

### What it is
Pandora is a load generator for Yandex.Tank performance test service.
Yandex.Tank - https://yandextank.readthedocs.io
Pandora - https://yandextank.readthedocs.io/en/latest/core_and_modules.html#pandora

### Project structure

* `cmd` - folders with applications
* `configs` - default configuration files
* `internal` - logic, specific for the pandora-application and which cannot be shared for other projects
* `scripts` - Go-scripts which are used in build-time
* `tests` - tests for the guns
