# Bootstrap Infra Pod Repository

## Building

```bash
cd ${image_dir}
ya package package.json --docker --docker-registry cr.yandex --docker-repository yc-bootstrap
```

## Pushing

```bash
cd ${image_dir}
ya package package.json --docker --docker-registry cr.yandex --docker-repository yc-bootstrap --docker-push
```

## Base Images

### Fluentd

Based on https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/api/fluent/docker

### Push-client

Based on https://a.yandex-team.ru/arc/trunk/arcadia/cloud/platform/selfhost/docker/push-client

## [Registry](https://console.cloud.yandex.ru/folders/b1gtdt9ccussdjo99746/container-registry/crpfc9niqhgtkgjjmu7b)

Production registry installation (cr.yandex) is used for storing images.
It uses the same cloud and folder as service vms:

yandexcloud (`b1gph5jh6bg8mjocuo4t`)/dogfood (`b1gtdt9ccussdjo99746`)

The command used to create registry is:
```bash
yc container registry create --cloud-id b1gph5jh6bg8mjocuo4t --folder-id b1gtdt9ccussdjo99746 --name bootstrap
```

Authentication: https://cloud.yandex.com/docs/container-registry/operations/authentication

Registry has shorthand alias `yc-bootstrap` see: https://st.yandex-team.ru/YCLOUD-2297

### Service accounts for nodes

#### dev (read-only)
Command used for creation:
```bash
yc iam service-account create --name bootstrap-container-registry-reader-dev-sa \
    --description "Service account for reading images from cr.yandex/yc-bootstrap container registry for dev environment"
yc container registry add-access-binding yc-bootstrap --role container-registry.images.puller \
    --subject serviceAccount:ajemb98s2h5ncr7cgbjc
yc iam key create --service-account-name bootstrap-container-registry-reader-dev-sa -o key.json
```
id: `ajemb98s2h5ncr7cgbjc`

### Service account for ci
```bash
yc iam service-account create --name bootstrap-container-registry-uploader-ci \
    --description "Service account for pushing images from cr.yandex/yc-bootstrap container registry for CI"
yc container registry add-access-binding yc-bootstrap --role container-registry.images.pusher \
    --subject serviceAccount:ajeeoq2npj92nfi2ufpr
yc iam key create --service-account-name bootstrap-container-registry-uploader-ci -o key.json
```
