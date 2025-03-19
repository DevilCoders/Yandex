# Deploying osquery-sender

## Preparing the environment

1. Install python2 library urllib3, e.g. by running
```
$ pip install urllib3
```
2. Get YAV OAuth token via https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982 and put it into `~/.ssh/yav_token`
3. Install the SKM: https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/
4. Run `./osquery-sender/scripts/terraform-init.sh`

## Deploying the new version or updated config

1. Update `application_version` in `variables.tf` or update the config in files/
2. Run `./osquery-sender/scripts/run-deploy.sh` (you may need to re-run `terraform-init.sh` if the terraform plan step fails).

## Deploying the new secret (e.g. certificate)

1. Run `./osquery-sender/scripts/skm-encrypt.sh`
2. Run `./osquery-sender/scripts/run-deploy.sh` (you may need to re-run `terraform-init.sh` if the terraform plan step fails).
3. Run `./osquery-sender/scripts/run-deploy.sh --reboot all` (SKM decrypts secrets only upon booting).

## Updating load balancer (including target list)

The main update script `./osquery-sender/scripts/run-deploy.sh` will fail if load balancer must be updated (e.g. the list of instances changed).

1. Run `./osquery-sender/scripts/run-deploy.sh --apply-lb` to check the proposed changes and apply them.
