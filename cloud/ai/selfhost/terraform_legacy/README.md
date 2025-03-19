## Deploy installations
Please if you deploy using this flow first time read first information below.

* Update variables in preprod installation
* Deploy preprod
* Run tests
* Update variables in prod installation
* Create PR for changes
    ```bash
    ya pr create -m 'Deploy cloud ai <TIKET_ID>'
    ```
* Deploy prod

### update any installation in prod/preprod
```bash
# Change dir to installation dir, example:
cd cloud/ai/selfhost/terraform/preprod/stt
# Ensure that you executer ./scripts/terraform-init.sh before and run terraform apply
./scripts/terraform.sh apply
# Start update of IG
./scripts/ig-update.sh
```

## General
See cloud/platform/selfhost/terraform/README.md for the general information.

## Setup

Install terraform, by running
```bash
brew install terraform
```

Install python2 libraries urllib3 and requests, e.g. by running
```bash
pip2 install urllib3 requests
```

Install ycp terraform provider
```bash
curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
```

Install skm
```bash
curl https://storage.cloud-preprod.yandex.net/skm/darwin/skm > ~/skm/skm
```
Add it to the PATH

Setup yc cli using the following instruction: https://cloud.yandex.ru/docs/cli/quickstart

## Initialization

1. Get YAV OAuth token via https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982 and put it into `~/yav_token`

2. Run the following command from the application subdirectory (for example from ./prod/stt)
```bash
./scripts/terraform-init.sh
```

## Running

Run
```bash
./scripts/terraform.sh plan
```
to see the plan of changes.

Run
```bash
./scripts/terraform.sh apply
```
to apply the changes.

## Destroy

Run
```bash
./scripts/terraform.sh destroy
```
to destroy allocated resources.

## Important

- Each time when you changed modules that are used need to call ./scripts/terraform-init.sh again, if you changed files that are used by modules or files of your main config reinit is not necessary
- If you copy-paste terraform environment remove manually ".terraform" directory
