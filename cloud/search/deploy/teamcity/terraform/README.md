# Terraform config for YC Search

## Usage
In order to use this config you need to provide vars (to access to s3).
So get secret_value from yav https://yav.yandex-team.ru/secret/sec-01g2fvpyyxkqjbkv5pj5z1ej43/explore/version/ver-01g2fvpyz8t0yfcs3v6mvy75tx
and do:

    cd israel/
    terraform init -backend-config="secret_key={secret_value}"

### Run plan
#### Prerequisites
To use environment variables from .envrc install and run terraform under https://direnv.net/
#### Run
To run plan you need initialized terraform:

    terraform init -backend-config="secret_key={secret_value}"

To see changes:

    terraform plan
To apply changes:

    terraform plan -out out
    terraform apply "out"

## Deploy new yc search component version
1. Update version in `envs/yc-prod/yc_search_versions.tf` file
2. Run `terraform plan -out out`
3. Check diff (there should be only bood_disk image_id changes)
4. Run `terraform apply out`
5. Watch your services at [console](https://console.cloudil.co.il/folders/yc.search.backends/compute/instance-groups)
6. Enjoy!
