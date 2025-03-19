# Как настроить локальный terraform

Копируем себе ключ
```
ya vault get version sec-01ecmcrsn5t7jr0f6ks8hz8p1c -o sa_key > ~/sa-key/l7.json
```
(Путь может быть другой, то тогда надо указать его в переменной TF `gpn_sa_file`.)

Вызываем `./init.sh`.

Возможно надо поставить/обновить провайдер,
на Linux:
```
$ curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
```
на Mac:
```
$ brew upgrade terraform-provider-ycp
```

# Как настроить локальный ycp

Правим конфиг `~/.config/ycp/config.yaml`:

```
profiles:
  ...
  gpn-l7:
    user: gpn-l7
    environment: gpn
    cloud-id: "bn3rfvtr44kn892nnpim"
    folder-id: "bn3v8a9sj6u8ih3a088r"
    format: yaml

users:
  ...
  gpn-l7:
    # Ключ берем из
    #
    # ya vault get version sec-01ecmcrsn5t7jr0f6ks8hz8p1c -o sa_key | jq
    #
    service-account-key: {
      "id": "d3bmpig9rogliokhn8q7",
      "service_account_id": "d3btkdr3p6oud69s3cv9",
      ...
    }

environments:
  gpn:
    # Берем из terraform/gpn/ycp-config.yaml
    resource-manager:
      endpoint:
        address: '[2a0d:d6c0:200:100:9a03:9bff:fea7:2241]:18443'
        plaintext: true
    ...
```

# Как прокатить изменения

```
./init.sh
terraform plan | ../../tf-grep.sh
terraform apply # лучше с --target что.катим
```

# Как положить свой ssh-ключ

Добавляем в `terraform/gpn/l7/common/default_ssh_keys.txt`.
Делаем `terraform apply`.

# Где взять ID IG, адреса и т.п.

`./status.sh`
или смотрим в нём команды ycp и вызываем их руками.

# Troubleshooting

## Не зайти после старта

Получаем список ID VM
```
$ ycp compute instance list | yq -c '.[]|[.id, .name, .network_interfaces[].primary_v6_address]'
```

Смотрим serial output
```
$ ycp compute instance get-serial-port-output ckko65jem6g0fthap79k | yq -r .contents
```

## После старта ничего не работает

`sudo systemctl` - не должно быть красного.
`sudo systemctl status config-provisioner` - он должен быть `active (exited)`

Энвой отвечает на хелсчеки
```
$ curl localhost:30080/ready
LIVE
```

Если энвой работает - смотрим на L3 (`./status.sh`)
