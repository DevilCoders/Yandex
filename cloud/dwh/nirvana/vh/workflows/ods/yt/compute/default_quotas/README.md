##### Описание таблицы

Дописывает в таблицу дефолтные квоты ('default_quotas') новые данные.
Ручная загрузка данных из https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/common/compute.sls#72

##### Загрузка

Периодичность загрузки: каждый час 
Загрузка: ручная
Теги: 

Prod:
[SOURCE PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/compute/default_quotas)
/ [TARGET PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/compute/default_quotas)

Preprod:
[SOURCE PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/compute/default_quotas)
/ [TARGET PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/compute/default_quotas)

##### Схема

| target column |  target column type | description | other |
|--|--|--|--|
| `quota_name` | String | Имя квоты | |
| `quota_limit`| Int64 | Максимально допустимое значение | |
| `unit`       | String| единица изменерения | |
| `start_at`   | Datetime | Дата время начала действия дефолтной квоты | |
| `end_at`     | Datetime | Дата время окончания действия дефолтной квоты | |

##### Проверки качества данных
