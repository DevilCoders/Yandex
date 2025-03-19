## General
See ../../README.md for the general information.

## Installation

Install python2 library urllib3, e.g. by running
```
$ pip install urllib3
```

## Initialization

1. Get Yandex Cloud OAuth token via https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb and put it into `~/.ssh/yc_token`

2. Get YAV OAuth token via https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982 and put it into `~/.ssh/yav_token`

3. Run the following command:
```
$ ./scripts/terraform-download-plugin.sh
```

4. Run the following command from the `certificate-manager-control-plane` and `certificate-manager-data-plane` subdirectories
```
$ ./scripts/terraform-init.sh
```

## Running

Run
```
$ ./scripts/terraform.sh plan
```
to see the plan of changes.

Run
```
$ ./scripts/terraform.sh apply
```
to apply the changes.
