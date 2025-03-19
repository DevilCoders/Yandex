## Staff Persons
#ods #staff #persons #PII

Данные о сотрудниках из стаффа.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
4. [Структура - PII.](#структура---PII)
5. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                       | Расположение данных - PII                                                                                         | Источники                                                                                                     |
|---------|-----------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| PROD    | [persons](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/staff/persons)    | [persons-PII](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/staff/PII/persons)    | [raw-persons](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/staff/persons)    |
| PREPROD | [persons](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/staff/persons) | [persons-PII](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/staff/PII/persons) | [raw-persons](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/staff/persons) |


### Структура
| Поле                   | Описание                                                                                    |
|------------------------|---------------------------------------------------------------------------------------------|
| staff_user_hash        | ID в стаффе, хеш                                                                            |
| staff_user_id          | ID в стаффе (Int)                                                                           |
| staff_user_uid         | Guid в стаффе                                                                               |
| staff_user_login_hash  | логин сотрудника на стаффе, хеш                                                             |
| firstname_en_hash      | Имя на английском, хеш                                                                      |
| firstname_ru_hash      | Имя на русском, хеш                                                                         |
| lastname_en_hash       | Фамилия на английском, хеш                                                                  |
| lastname_ru_hash       | Фамилия на русском, хеш                                                                     |
| yandex_login_hash      | Логин привязанной учетки на yandex.ru, хеш                                                  |
| is_deleted             | флаг, удален ли сотрудник                                                                   |
| official_affiliation   | Компания, в которую зачислен сотрудник                                                      |
| official_quit_dt       | Дата увольнения.                                                                            |
| official_is_dismissed  | Признак, уволен.                                                                            |
| official_is_homeworker | Признак, работает из дома.                                                                  |
| official_is_robot      | Признак, робот.                                                                             |
| department_ancestors   | информация о лестнице департаментов сотрудника.                                             |
| department_id          | ID департамента, в котором сотрудник непосредственно работает.                              |
| department_name        | Название департамента, в котором сотрудник непосредственно работает.                        |
| department_url         | url департамента, в котором сотрудник непосредственно работает.                             |
| department_fullname    | Название департамента с учетом департаментов выше по лестнице (начинается с уровня облака). |
| cloud_department_id    | id департамента в облаке (самый крупный раздел внутри облака).                              |
| cloud_department_name  | название департамента в облаке (самый крупный раздел внутри облака).                        |
| cloud_department_url   | url департамента в облаеке (самый крупный раздел внутри облака).                            |


### Структура - PII
| Поле             | Описание                              |
|------------------|---------------------------------------|
| staff_user_hash  | ID в стаффе, хеш                      |
| staff_user_id    | ID в стаффе (Int)                     |
| staff_user_uid   | Guid в стаффе                         |
| staff_user_login | логин сотрудника на стаффе            |
| firstname_en     | Имя на английском                     |
| firstname_ru     | Имя на русском                        |
| lastname_en      | Фамилия на английском                 |
| lastname_ru      | Фамилия на русском                    |
| yandex_login     | Логин привязанной учетки на yandex.ru |
| official_quit_dt | Дата увольнения.                      |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
