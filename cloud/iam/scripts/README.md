See yc-secret-cli documentation for details https://wiki.yandex-team.ru/cloud/infra/secret-service/cli/

Config ~/.config/yc-secret-cli/config.yaml
```yaml
current: prod
profiles:
  preprod:
    endpoint: secrets.cloud-preprod.yandex.net:8888
  prod:
    endpoint: secrets.cloud.yandex.net:8888
  testing:
    endpoint: secrets.cloud-testing.yandex.net:8888

```

Usage example:
```
./create_test_secret /tmp/server.pem yc-access-service
```

## readd_svm.sh

Script to delete and re-add an SVM by its FQDN.
This script will automatically set and remove the downtime for the host
and update the ticket with results.

### Prerequisites

* File **~/.config/oauth/st** - [OAUTH token](
  https://oauth.yandex-team.ru/authorize?response_type=token&client_id=5f671d781aca402ab7460fde4050267b)
  for https://st.yandex-team.ru/
* File **~/.config/oauth/juggler** - [OAUTH token](
  https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0)
  for https://juggler.yandex-team.ru/
* Environment variable **TICKET**

### Example

```bash
TICKET=CLOUD-XXXX ./readd_svm.sh iam-sas1.svc.cloud-preprod.yandex.net
```
