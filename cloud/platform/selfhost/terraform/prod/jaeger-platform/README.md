# Serverless deployment

## Prerequisites
Install:
* `terraform`
  * Optionally, install `terraform-landscape` for better changes overview
* [yatool](https://wiki.yandex-team.ru/yatool/)
* `jq`
* `awscli`

Export `YC_TOKEN` variable to `env`: you can get proper auth URL [here](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb).

## Deploy steps
1. Init Terraform:
    ```sh
    terraform init -backend-config="secret_key=$(ya vault get version sec-01cwtqr1fsz56yx166hj319sqb -o secret_key)" 
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

Update versions.tf when new versions are to be deployed.

Do not edit `files/docker-json.tpl` and `state.tf`.

Refer to [guide](https://wiki.yandex-team.ru/users/zverre/Rukovodstvo-po-vykatke-i-obsluzhivaniju-Jaeger/) in case you have run into problems or don't know what to do.

Secrets can be obtained via [`yav` module](../../modules/yav)
