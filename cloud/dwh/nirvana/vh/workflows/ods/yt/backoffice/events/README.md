#### Описание

Вычитывает последний (актуальный) снапшот таблицы `events`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/events)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/events)

- `id`                              - ID мероприятия
- `name_ru`                         - Название мероприятия на русском языке
- `name_en`                         - Название события на английском языке
- `description_ru`                  -
- `description_en`                  -
- `short_description_ru`            -
- `short_description_en`            -
- `date_msk`                        - (временная зона MSK (UTC+3:00))
- `created_at_msk`                  - Дата и время создания (временная зона MSK (UTC+3:00))
- `updated_at_msk`                  - Дата и время последнего обновления (временная зона MSK (UTC+3:00))
- `image`                           -
- `record`                          -
- `stream`                          -
- `place_id`                        -
- `is_canceled`                     -
- `url`                             -
- `is_published`                    -
- `start_registration_time`         -
- `registration_status`             -
- `registration_form_hashed_id`     -
- `is_online`                       -
- `webinar_url`                     -
- `custom_registration_form_fields` -
- `need_request_email`              -
- `is_private`                      -
- `will_broadcast`                  -
- `contact_person_hash`             - хеш ФИО контактного лица для события
- `contact_person_phone_hash`       - хеш номера телефона контактного лица

Personally Identifiable Information (PII): [PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/PII/events)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/PII/events)

- `id`                   - ID мероприятия
- `contact_person`       - ФИО контактного лица для события
- `contact_person_phone` - Номер телефона контактного лица
