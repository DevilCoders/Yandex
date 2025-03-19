## Staff Persons
#raw #staff #persons

Выгрузка сотрудников облака и роботов из [апи стаффа](https://staff-api.yandex-team.ru/v3/persons?_doc=1).
Сотрудниками облака считаются все, у кого в лестнице организационной структуры есть департамент с ID = [70618](https://staff.yandex-team.ru/departments/yandex_exp_9053/).
Роботами считаются все, у кого в лестнице организационной структуры есть департамент с ID = [2073](https://staff.yandex-team.ru/departments/virtual_robots/).
При запросе к api используется [pql](https://github.com/alonho/pql).


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                       | Источники                                              |
|---------|-----------------------------------------------------------------------------------------------------------|--------------------------------------------------------|
| PROD    | [persons](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/staff/persons)    | [API](https://staff-api.yandex-team.ru/v3/persons?_doc=1)     |
| PREPROD | [persons](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/staff/persons) | [API](http://staff-api.test.yandex-team.ru/v3/persons?_doc=1) |


### Структура
| Поле                   | Описание                               |
|------------------------|----------------------------------------|
| поле                   | описание                               |
| id                     | ID в стаффе (Int)                      |
| uid                    | Guid в стаффе                          |
| login                  | Логин сотрудника на стаффе             |
| firstname_en           | Имя на английском                      |
| firstname_ru           | Имя на русском                         |
| lastname_en            | Фамилия на английском                  |
| lastname_ru            | Фамилия на русском                     |
| is_deleted             | Флаг, удален ли сотрудник              |
| official_affiliation   | Компания, в которую зачислен сотрудник |
| official_quit_at       | Дата увольнения                        |
| official_is_dismissed  | Признак, уволен                        |
| official_is_homeworker | Признак, работает из дома.             |
| official_is_robot      | Признак, робот.                        |
| yandex_login           | Логин привязанной учетки на yandex.ru  |
| department_group       | информация о департаменте сотрудника.  |
| load_ts                | дата и время загрузки данных, UTC.     |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
