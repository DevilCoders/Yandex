# Registries for IAAS container images

## Getting started
1. Make .terraformrc as mentioned in https://clubs.at.yandex-team.ru/ycp/4790
2. Init terraform
```shell
terraform init -backend-config secret_key=$(ya vault get version sec-01dhndngvxc5pacfrkdbxg9ahh -o s3_mds_service_id_1415_secret_access_key)
```

## Plan
```shell
YC_TOKEN=$(yc --profile=prod iam create-token) terraform plan
```

## Apply
```shell
YC_TOKEN=$(yc --profile=prod iam create-token) terraform apply
```
