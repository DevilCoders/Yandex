# Порядок применения бандла

Ниже приведена пошаговая инструкция по применению бандла в среде

### Получение скрипта для скачивания

Необходимо найти в консоли бакет S3, соответствующий среде:
 - [preprod](https://console-preprod.cloud.yandex.ru/folders/yc.billing.service-folder/storage/buckets/yc-billing-configs/selfsku)
 - [prod](https://console.cloud.yandex.ru/folders/yc.billing.service-folder/storage/buckets/yc-billing-configs/selfsku)
 - [ya-preprod](https://console.cloud.yandex.ru/folders/b1gonbqi1906m7cnto0n/storage/buckets/yc-yandex-billing-preprod-configs/selfsku-ya)
 - [ya-prod](https://console.cloud.yandex.ru/folders/b1gonbqi1906m7cnto0n/storage/buckets/yc-yandex-billing-preprod-configs/selfsku-ya)

После чего получить ссылку для скачивания download.sh

![](_assets/s3-link.png)

> Если консоль недоступна, то можно взять скрипт из аркадии
> - [ya-preprod](https://a.yandex-team.ru/arc_vcs/cloud/billing/bootstrap/internal/packaging/scripts/download_preprod.sh)
> - [ya-prod](https://a.yandex-team.ru/arc_vcs/cloud/billing/bootstrap/internal/packaging/scripts/download_prod.sh)

### Скачивание и применение бандла

Необходимо подключиться к svm по ssh (use pssh + wiki instructions)

На svm надо:
 - создать отдельный каталог для бандлов
 - скачать туда скрипт и сделать исполняемым (предварительно убедившись в безопасности)
 - скачать бандлы скриптом
 - для каждого полученного бандла:
   - создать каталог для распаковки файлов
   - распаковать в него файлы
   - убедиться что файлы корректно создались
   - применить полученные файлы

```
> DOWNLOADER_URL='<s3_link_here>'
> mkdir -p selfsku && cd selfsku && rm -r *
> curl $DOWNLOADER_URL -o download.sh && chmod a+x download.sh
> ./download.sh
> mkdir -p common_bundle && ./common extract common_bundle && sudo yc-billing-cli sku populate -d common_bundle
```
