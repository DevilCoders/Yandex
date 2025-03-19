### Base salt formula for oslogin

Based of https://github.com/GoogleCloudPlatform/guest-oslogin

##### create test instance:
```bash
$ export YC_TOKEN=$(yc --profile preprod iam create-token)
$ terraform init
$ terraform plan
$ terraform apply
```
