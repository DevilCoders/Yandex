## Preconditions

There are some requirements to the environment in which release helping scripts are executed:

* installed packages - jq, [aws cli](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html) (for s3)
* installed ycp version `0.3.0-2022-03-29T17-09-00Z+8565285b14` or newer
* configured ycp profile for each stand - [example](https://paste.yandex-team.ru/10220359)
* OAuth tokens for Juggler (downtimes) and StarTrek
* Configured aws profile for iam s3 bucket, credentials are in [lockbox](https://console.cloud.yandex.ru/folders/b1gggp9mqe70mtrng2qu/lockbox/secret/e6qli9h4q1kovkarehkh/overview), init script is [here](https://paste.yandex-team.ru/9756020)

Script to check known preconditions:
```bash
cd $(dirname $(readlink $(which ya)))/cloud/iam/scripts/metadata_release
source ./env.sh
./preconditions.sh
```

## Release ticket

```shell
cd ~/go/src/bb.yandex-team.ru/cloud/cloud-go/ # or another cloud-go location
git checkout master && git pull # or another commit from which to perform a release

ARC_ROOT=$(dirname $(readlink $(which ya)))
cd $ARC_ROOT/cloud/iam/scripts/metadata_release
./prepare_ticket.py
```
Run `metadata_release/prepare_ticket.py`. It will 
* find the previous release ticket via ST API, 
* gather changelog from the local cloud-go repository,
* open a ticket template url in the default browser.

### OR

Create a release ticket using [this template](https://st.yandex-team.ru/createTicket?queue=CLOUD&components%5B%5D=2369&components%5B%5D=39829&description=Previous+ticket+-+CLOUD-XXXXXX%0A%60%60%60%0ACURRENT_FIXTURES_HASH%3Daaaaaa%0APREVIOUS_FIXTURES_HASH%3Dbbbbbb%0A%60%60%60%0A%3C%7Bcommits+with+changes+to+%60private-api%2Fyandex%2Fcloud%2Fpriv%2F**.yaml%60+that+are+included+to+this+release%0A%60%60%60%0A%23+TODO%3A%0Agit+log+--oneline+%24%7BPREVIOUS_FIXTURES_HASH%3F%7D..%24%7BCURRENT_FIXTURES_HASH%3F%7D+--+%27private-api%2Fyandex%2Fcloud%2Fpriv%2F**.yaml%27%0A%0A%60%60%60%0A%7D%3E%0A%3C%7BTicket+list+for+linking%0A%23+TODO%3A%0Agit+log+--oneline+%24%7BPREVIOUS_FIXTURES_HASH%3F%7D..%24%7BCURRENT_FIXTURES_HASH%3F%7D+--+%27private-api%2Fyandex%2Fcloud%2Fpriv%2F**.yaml%27+%7C+grep+-Po+%27%5Cb%5BA-Z%5D%7B3%2C%7D-%5Cd%2B%5Cb%27+%7C+sort+%7C+uniq%0A%7D%3E%0A&priority=2&summary=Deploy+IAM+metadata+%24DATE%24&tags%5B%5D=duty&type=12&checklistItems%5B%5D=%7B%22text%22%3A%22internal-dev%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22testing%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22internal-prestable%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22preprod%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22internal-prod%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22prod+node%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22prod+zone%22%2C%22checked%22%3Afalse%7D&checklistItems%5B%5D=%7B%22text%22%3A%22prod+all%22%2C%22checked%22%3Afalse%7D), 
specify change date in the summary and collect the changelog:

```bash
cd ~/go/src/bb.yandex-team.ru/cloud/cloud-go/ # or another cloud-go location
git checkout master && git pull # or another commit from which to perform a release

CURRENT_FIXTURES_HASH=6c0ae3bd2 # git rev-parse HEAD
PREVIOUS_FIXTURES_HASH=acd5c99879 # take from the previous release ticket

# list of commits:
git log --oneline ${PREVIOUS_FIXTURES_HASH?}..${CURRENT_FIXTURES_HASH?} -- 'private-api/yandex/cloud/priv/**.yaml'

# list of tickets:
git log --oneline ${PREVIOUS_FIXTURES_HASH?}..${CURRENT_FIXTURES_HASH?} -- 'private-api/yandex/cloud/priv/**.yaml' | grep -Po '\b[A-Z]{3,}-\d+\b' | sort | uniq
```

## Review changes

Manually verify that execution plan for production looks good:

```bash
ycp --profile prod iam inner metadata update --dry-run
# --short-diff flag may be used to collapse arrays and review other changes more carefully
ycp --profile prod iam inner metadata update --dry-run --short-diff
```

If some problems found, fix them, wait for PR to be merged and repeat.

## Prepare release artifacts

```bash
cd $(arc root)/cloud/iam/scripts/metadata_release
source ./env.sh
export TICKET=CLOUD-XXXXX
./prepare_release.sh
```

This will prepare two yaml files for each stand/profile:
 * ${PROFILE}_dump.yaml - desired states from source yaml files
 * ${PROFILE}_export.yaml - current state fetched from API

The main purpose of dump files is to make a snapshot on a specific cloud-go commit for further usage.

Export file is needed for:
* Faster dry runs without fetching all data from API each time.
* Ability to quickly rollback to pre-release using `--from-file` flag of `ycp iam inner metadata update` command.
* Making diff for any purpose - find distinctions between environments or releases.

`./prepare_release.sh` will also upload and post a comment about that to the ST ticket.

## Perform release

```bash
cd $(arc root)/cloud/iam/scripts/metadata_release
export TICKET=CLOUD-XXXXX
source ./env.sh

# day 1 - internal-dev, YC-testing, internal-prestable, YC-preprod, DC-preprod
./release.sh internal-dev
../refresh_as_caches.sh C@cloud_prod_iam-internal-dev 

./release.sh testing
../refresh_as_caches.sh C@cloud_testing_iam-as

./release.sh internal-prestable
../refresh_as_caches.sh C@cloud_prod_iam-internal-prestable

./release.sh preprod
../refresh_as_caches.sh C@cloud_preprod_iam-as

./release.sh doublecloud-aws-preprod
# TODO kubectl --context datacloud-aws-preprod -n iam rollout restart deployment access-service

# day 2 - internal-prod
./release.sh internal-prod
# on production stands for now do both - refresh and restart. Later (~ since 2022-07-18) we'll only leave refresh.
../refresh_as_caches.sh 'C@cloud_prod_iam-ya[0]' # in the morning
../restart_as.sh 'C@cloud_prod_iam-ya[0]' # in the morning
../refresh_as_caches.sh 'C@cloud_prod_iam-ya[1:]' # after 4 hours
../restart_as.sh 'C@cloud_prod_iam-ya[1:]' # after 4 hours

# day 3 - YC-prod, one myt node; IL, one il1 node; DC-prod
./release.sh prod
../refresh_as_caches.sh 'C@cloud_prod_iam-as_myt[0]'
../restart_as.sh 'C@cloud_prod_iam-as_myt[0]'

./release.sh israel
../refresh_as_caches.sh iam-as-il1-a1.svc.yandexcloud.co.il
../restart_as.sh iam-as-il1-a1.svc.yandexcloud.co.il

./release.sh doublecloud-aws-prod
# TODO kubectl --context datacloud-aws-prod -n iam rollout restart deployment access-service

# day 4 - YC-prod, remaining myt nodes; IL, remaining il1 nodes
../refresh_as_caches.sh 'C@cloud_prod_iam-as_myt[1:]'
../restart_as.sh 'C@cloud_prod_iam-as_myt[1:]'
../refresh_as_caches.sh 'r@iam-as-il1-a2...3.svc.yandexcloud.co.il'
../restart_as.sh 'r@iam-as-il1-a2...3.svc.yandexcloud.co.il'

# day 5 - YC-prod, vla and sas
../refresh_as_caches.sh C@cloud_prod_iam-as_vla
../restart_as.sh C@cloud_prod_iam-as_vla
../refresh_as_caches.sh C@cloud_prod_iam-as_sas
../restart_as.sh C@cloud_prod_iam-as_sas
```

## FAQ / snippets

### Revert the release

```shell
PROFILE=PROD
# run with export.yaml and dump.yaml files swapped
ycp --profile=$PROFILE iam inner metadata update \
  --from-file=./current/${PROFILE}_export.yaml \
  --export-file=./current/${PROFILE}_dump.yaml
```

Note that newly created entities will not be reverted, because deletion operations are not supported now.

### Fetch release artifacts to continue on another machine

```shell
# define an alias
s3_mds="aws --profile iam-metadata-release --endpoint-url https://s3.mds.yandex.net s3"
# list release dates
$s3_mds ls s3://iam/metadata/releases/
# list release hashes (usually there's one) 
$s3_mds ls s3://iam/metadata/releases/2022-04-25/
# copy files
$s3_mds cp --recursive s3://iam/metadata/releases/2022-04-25/11ac3a7d/ ./current/
```
