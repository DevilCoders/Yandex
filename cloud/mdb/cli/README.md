## Overview

MDB admin tools.

## Requirements

Supported platforms are Mac OS and Linux. It's also required to install `s3cmd` and configure [`ya vault`](https://vault-api.passport.yandex.net/docs).

## Installation

Run `./install.sh` to install tools and configure shell completion.

The install script supports multiple useful options. See help for details.

```
$ cli/install.sh --help
Usage: install.sh [<option>] ...
  -t, --tool <value> Tool to install. Possible values: "dbaas", "mdb-admin" and "all". Default is "all".
  -p, --path <value> Install path. Can be set using environment variable DBAAS_TOOL_ROOT_PATH. Default is "~/mdb-scripts".
  -a, --auto         Suppress questions and perform installation in automatic mode.
  -h, --help         Show this help message and exit.
```

## Usage

See tool specific instructions.

- [dbaas](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/cli/dbaas/README.md)
- [mdb-admin](https://wiki.yandex-team.ru/mdb/internal/operations/tools/#mdb-admin)

## Contribution

Feedback and PRs are welcome.

After committing your changes use [this Jenkins job](https://jenkins.db.yandex-team.ru/view/all/job/arcadia-mdb-scripts-release/) to release new version.
