## billing metrics
'#billing #metrics

Логи биллинга, данные с 01.01.2021

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                            | Источники                                                                                                                                    |
|---------|--------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [billing_metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/billing_metrics/1d)    | [raw_billing_metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logfeller/billing/billing_metrics/1d)    |
| PREPROD | [billing_metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/billing_metrics/1d) | [raw_billing_metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logfeller/billing/billing_metrics/1d) |



### Структура
| Поле                             | Описание                                                                                       |
|----------------------------------|------------------------------------------------------------------------------------------------|
| id                               | billing_record_id                                                                              |
| cloud_id                         | [id](../../iam/clouds) облака                                                                  |
| cluster_id                       | [id](../../mdb/clusters) кластера                                                              |
| cluster_type                     | [тип](../../mdb/clusters) кластера                                                             |
| compute_instance_id              | id инстанса                                                                                    |
| core_fraction                    | доля ядер                                                                                      |
| cores                            | кол-во ядер на инстансе                                                                        |
| disk_size                        | размер диска                                                                                   |
| disk_type_id                     | id типа диска                                                                                  |
| folder_id                        | [id](../../iam/folders) фолдера                                                                |
| gpus                             | кол-во gpu                                                                                     |
| host_id                          | host_id                                                                                        |
| eventtime_dttm_local             | дейттайм события (Мск)                                                                         |
| eventtime_ts                     | таймстемп события                                                                              |
| managed_kubernetes_cluster_id    | id кластера кубера                                                                             |
| managed_kubernetes_node_group_id | id хост группы кластера кубера                                                                 |
| memory                           | RAM                                                                                            |
| online                           | uptime vm                                                                                      |
| platform_id                      | платформа                                                                                      |
| preemptible                      | является ли vm [preemptible](https://cloud.yandex.com/en/docs/compute/concepts/preemptible-vm) |
| product_ids                      | id продукта в маркетплейсе                                                                     |
| public_fips                      | ?                                                                                              |
| vm_id                            | vm_id                                                                                          |
| resource_preset_id               | [flavor_id](../../mdb/flavors)                                                                 |
| schema                           | только схемы 'compute.vm.generic.v1' и 'mdb.db.generic.v1'                                     |
| usage_finish                     | конец использования vm                                                                         |
| usage_quantity                   | кол-во секунд использования vm                                                                 |
| usage_start                      | начало использования vm                                                                        |
| usage_type                       | везде "delta"                                                                                  |
| usage_unit                       | везде "seconds"                                                                                |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.
