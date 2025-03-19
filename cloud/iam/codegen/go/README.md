# Generate IAM constants for cloud-go

0. Download [fixture_permissions.yaml](https://teamcity.yandex-team.ru/repository/download/Cloud_CloudGo_IamCompileRoleFixtures/.lastFinished/fixture_permissions.yaml).
0. Upload the file to Sandbox: `ya upload --ttl=inf -T=PLAIN_TEXT ~/Downloads/fixture_permissions.yaml`
0. Update the returneed resource id in all nested `ya.make` files (`permissions`, `quotas`, `resources` directories): `FROM_SANDBOX(FILE <new resource_id> OUT_NOAUTO fixture_permissions.yaml)`
0. Run `GOPATH=~/go ./copy_to_bitbucket.sh`. Skip the `GOPATH` variable if you have it set system-wide.

# Todo

Automate this task more.
Get inspiration from [java codegen](https://a.yandex-team.ru/svn/trunk/arcadia/cloud/iam/codegen/java/fixtures).
