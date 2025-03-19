# Сборка Compute-образа HashiCorp Vault с поддержкой Auto Unseal via YC KMS

Чтобы собрать образ нужно:
1) Подготовить ветку с новой версией в https://github.com/yandex-cloud/vault
2) Собственно собрать образ
3) Протестировать полученный образ

## Подготовка ветки с новой версией
В https://github.com/yandex-cloud/vault лежит форк официального репозитория HashiCorp Vault,
который располагается тут: https://github.com/hashicorp/vault

Отличия yandex-cloud-репозитория от hashicorp-репозитория:
* есть ветка [yckms-auto-unseal](https://github.com/yandex-cloud/vault/tree/yckms-auto-unseal), содержащая изменения,
ответсвенные за поддержку Auto Unseal via YC KMS в Vault. Автогенерируемые измененения (vendor, go.sum итп) в ветку не
  включены
* для каждого релиза нашего образа есть ветка вида vX.X.X+yckms, где vX.X.X –
  официальная версия Vault, на которой основана наша версия. То есть: vX.X.X+yckms == vX.X.X +
[коммиты из [yckms-auto-unseal](https://github.com/yandex-cloud/vault/tree/yckms-auto-unseal)]
В остальном yandex-cloud и hashicorp-репозитории ничем не отличаются, в том числе и master-ветками.
  
Чтобы подготовить новую ветку vX.X.X+yckms с изменениями для версии vX.X.X+yckms надо:
1) Подтянуть изменения из upstream'а (https://github.com/hashicorp/vault),
   см. [Syncing a fork](https://docs.github.com/en/github/collaborating-with-issues-and-pull-requests/syncing-a-fork)
1) Определиться с базовой официальной версией и ответвить от соответствующего официального тега
   вида vX.X.X новую ветку с именем vX.X.X+yckms
1) Подтянуть в ветку vX.X.X+yckms изменения из
   [yckms-auto-unseal](https://github.com/yandex-cloud/vault/tree/yckms-auto-unseal):
   `git cherry-pick 5ae8f053f9edf9fc05cc2381482ddd9c7740ced8^..yckms-auto-unseal --no-commit`
1) Выполнить: `go mod vendor`
1) В sdk/version/version_base.go в VersionMetadata указать "yckms": `VersionMetadata = "yckms"`
1) Закоммитить/запушить изменения

## Сборка образа
Чтобы собрать Compute-образ из подготовленной ранее ветки:
1) В install.sh в BRANCH указать название ветки
2) Выполнить `OAUTH_TOKEN=*** FOLDER_ID=b1g5769jul0r6h1acc91 packer build vault.packer.json`, указав в OAUTH_TOKEN
свой OAuth-токен, в FOLDER_ID – каталог, в который будет помещен собрнный образ
   
## Тестирование образа
Пока что выполняется в ручном режиме. Для тестирования:
* Разворачиваем виртулку с собранным ранее образом (можно прямо через консоль)
* Идём на созданную виртуалку по SSH
* Тестируем:
  1. Выполняем `vault version` – убеждаемся, что версия ожидаемая и имеет вид vX.X.X+yckms
  2. Выполняем `VAULT_ADDR=http://localhost:8200 vault status`, убеждаемся, что получили
     Initialized=false, Sealed=true и Version вида X.X.X+yckms
  2. В /etc/vault.d/vault.hcl раскоментируем блок seal "yandexcloudkms", указываем валидный KMS-ключ
     (убеждаемся, что у сервисного аккаунта, привязанного к виртуалке, есть роль kms.keys.encrypterDecrypter),
     рестартуем vault (`sudo systemctl restart vault`), убеждаемся, что `VAULT_ADDR=http://localhost:8200 vault status`
     отдает Initialized=false, Sealed=true
  2. С локальной машины прокидываем порт: `ssh -NL 8200:localhost:8200 <IPv4-виртуалки>`, в браузере открываем по URLу 
     http://localhost:8200 UI Vault'а:
       * Указываем в Key shares – 5, в Key threshold – 3, жмем Initialize
       * Через Download keys скачиваем части ключа и root token
       * Жмем Continue to authenticate
       * Указываем root token, который скачали ранее и жмем Sign In, должны успешно войти
  2. Рестартуем страницу в браузере, убеждаемся, что попадаем на страницу 'Sign in to Vault' и ничего кроме токена
     вводить не надо (например части ключа для схемы Шамира).
  2. Выполняем `VAULT_ADDR=http://localhost:8200 vault status`, убеждаемся, что получили
     Initialized=true, Sealed=false
  1. Отбираем у сервисного акканта, привязанного к виртуалке роль kms.keys.encrypterDecrypter и рестартуем Vault 
     (`sudo systemctl restart vault`).
     Убеждаемся, что `VAULT_ADDR=http://localhost:8200 vault status` недоступен, а в `sudo journalctl -uvault | less`
     появилась строка:
     'failed to encrypt with Yandex.Cloud KMS key - ensure the key exists and permission to encrypt the key is granted:
     error encrypting data: rpc error: code = PermissionDenied desc = Permission denied'
  1. Возвращаем сервисному аккаунту роль kms.keys.encrypterDecrypter и рестартуем Vault (`sudo systemctl restart vault`),
     выполняем `VAULT_ADDR=http://localhost:8200 vault status`, убеждаемся, что получили Initialized=true, Sealed=false
