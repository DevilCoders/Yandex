
## Getting Started & Documentation

If you're new to Terraform and want to get started creating infrastructure, please checkout our 
[Getting Started](https://www.terraform.io/intro/getting-started/install.html) guide, available on the 
[Terraform website](http://www.terraform.io).

All documentation is available on the [Terraform website](http://www.terraform.io):

  - [Intro](https://www.terraform.io/intro/index.html)
  - [Docs](https://www.terraform.io/docs/index.html)

## Quick Start

### Install
```sh
# check terraform installed
$ terraform version
  Terraform v0.12.5
  
```
### Init

```sh
# change directory to '<cloud_installation>/<you_service_name>'
$ cd cloud/platform/selfhost/terraform/preprod/container-registry
```

If there an `./init.sh` file, run it and it will do the terraform init.

If there is no `./init.sh`, run one of the following commands:
```sh                    
# init step: check current dir, download necessary providers, 
# get last state from S3 storage, init local state in dir '.terraform/' (should not be committed in VCS)

# Preprod installation:
$ terraform init -backend-config="secret_key=$(ya vault get version sec-01cwa619wea9r9ya23g61jdm8q -o secret_key)"

# Prod installation:
$ terraform init -backend-config="secret_key=$(ya vault get version sec-01cwtqr1fsz56yx166hj319sqb -o secret_key)"
``` 

### Usage
define env variable `YC_TOKEN` with 
[OAuth token](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb) 
for Yandex.Cloud
```sh
# plan step: analyze current cloud state and show necessary changes to get desired infrastructure run
$ terraform plan -var yc_token=$YC_TOKEN

# apply step: real apply changes to cloud, will show again planned change and ask for 'yes/no'?
$ terraform apply -var yc_token=$YC_TOKEN
```

in many cases you will also need `-var yandex_token=$YT_TOKEN`, where `$YT_TOKEN` is OAuth token in yandex-team 
services (with vault scope; you can get an auth string in `~/.docker/config.json` and `base64 -D` it OR 
[get new](https://vault-api.passport.yandex.net/docs/#oauth))

### DIY-fixes
* have you tried turning it off and on again?  
  You could remove `.terraform/` and `~/.terraform.d` (if exists) and try again `init` step
  ```sh  
  $ rm -rf .terraform/
  $ rm -rf ~/.terraform.d
  ```
* verbose logging  
  run terraform with `TF_LOG=trace`
  ```sh  
  $ TF_LOG=trace terraform plan -var yc_token=$YC_TOKEN
  ```


## Advise
Install [terraform-landscape](https://github.com/coinbase/terraform-landscape) for better diff in `plan` commands

### Update

Download new terraform from https://www.terraform.io/downloads.html

Update the binary:
```sh
$ unzip -p ~/Downloads/terraform_0.12.12_linux_amd64.zip terraform > `which terraform`
```

Run `init`.
