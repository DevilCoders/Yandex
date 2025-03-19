## Docker build & push

```shell
make docker-preprod
```

```shell
make docker-prod
```

## K8s infrastructure installation

```shell
make k8s-install-preprod
```

```shell
make k8s-install-prod
```

## K8s service deployment

```shell
DOCKER_IMAGE=cr.cloud-preprod.yandex.net/crt73rt5m8j28oujjv2c/connman:9157013 \
make k8s-deploy-preprod
```

```shell
DOCKER_IMAGE=cr.yandex/crpb5l8iv58ost7b3imq/connman:9157013 \
make k8s-deploy-prod
```

## Terraform commands

```shell
make tf-preprod plan
```

```shell
make tf-preprod apply
```

```shell
make tf-prod plan
```

```shell
make tf-prod apply
```