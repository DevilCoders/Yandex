# YCF deployment

## Prerequisites
Install:
* `terraform`
* [`yatool`](https://wiki.yandex-team.ru/yatool/)
* `jq`
* `awscli`
* `yc` (also, make sure you have 2 profiles named `prod` & `preprod` set up)
* [`terraform-provider-ycp`](https://wiki.yandex-team.ru/cloud/devel/terraform-ycp/)

Export `YC_TOKEN` variable to `env`: you can get proper auth URL [here](https://wiki.yandex-team.ru/cloud/devel/platform-team/secrets/).

## Deploy steps
1. Init Terraform:
    ```sh
    terraform init -backend-config="access_key=$(ya vault get version sec-01dqcthx3z6xrcfrbf42zgaf1p -o access_key)" -backend-config="secret_key=$(ya vault get version sec-01dqcthx3z6xrcfrbf42zgaf1p -o secret_key)" 
    ```
2. Plan changes:
    ```sh
    terraform plan -out plan.out | landscape
    ```
3. Apply changes:
    ```sh
    terraform apply plan.out
    ```

## Structure

Every sub-service should have its own `.tf` file.

To deploy VMs use `instance` submodule.

Do not edit `files/docker-json.tpl` and `state.tf`.

Secrets can be obtained via [`yav` module](../../modules/yav)
