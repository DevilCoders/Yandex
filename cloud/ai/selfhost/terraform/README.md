# YC AI: Selfhost Infrastructure as a code


## General
See:
* *cloud/platform/selfhost/terraform/README.md* for the general information about selfhosting
* *index.md* for detailed description about this folder organization and concepts
* *migration.md* for migration instruction from previous organization & terraform versions


## Setup
* **terraform** (version >= 1.0)<br>
  Recommended installation way via [tfenv](https://github.com/tfutils/tfenv) and specific version
* **python3** for *tfinit.py*
* [**skm**](https://wiki.yandex-team.ru/cloud/devel/platform-team/infra/skm/) in PATH
* **ya** tool in PATH and **ya vault** if configured


## Deploy
> IMPORTANT: Please follow deploy regulation of the specific services and create release tickets if  required


## About regulations
* Do not apply changes in infrastructure directly to production<br>
  Follow PREPROD -> STAGING -> PROD if it applied
* Better to commit code to repository before deployment


## Step by step instruction:

1. `tfinit.py <targets_group> <target> <environment>`<br>
    `<targets_group>/<target>/<environment>` here is effectively a path in source tree from *tfinit.py*<br>
    It initializes terraform for specific installation<br>
    And creates three files with secrets in folder specified above <br>
    * **backend.tfvars** - file with secrets required by teraform [backend](https://www.terraform.io/docs/language/settings/backends/s3.html)
    * **terraform.tfvar** - file with secrets required by terraform providers [yc](https://registry.terraform.io/providers/yandex-cloud/yandex/latest/docs#configuration-reference) and [ycp](https://wiki.yandex-team.ru/cloud/devel/terraform-ycp/)
    * **.yc_sa_key** - file with service account (referenced as deployer) private key which user impersonalises to perform actions with infractructure. Path to this file should be in **terraform.tfvar** 

2. Change your working directory to the `<targets_group>/<target>/<environment>`<br>
    Environment should be initialized and you can use standard terraform interaction such as:
    - **terraform plan** - check current changes in terraform against real infrastructure
    - **terraform apply** - apply current changes to real infrastructure
    - **terraform destroy** - destroy all resources in current target


## Important
- Each time when new modules are added into hierarchy init call of terraform is required<br>
  `terraform init -backend-config=backend.tfvars`
- If you copy-paste terraform environment remove ignored files to prevent bugs and reinit via *tfinit.py*<br>
  `git clean -fX`
