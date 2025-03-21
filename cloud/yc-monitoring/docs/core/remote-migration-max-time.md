[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=remote+migration+max+time)

## Описание
Срабатывает если время миграции между нодами превысило порог в 15 сек.

## Подробности
Миграция между нодами используется для расселения инстансов с ноды. Алерт говорит о том, что на ноде какой-то инстанс слишком долго переселяется.

Используем этот алерт для того чтобы дежурный по core получал сообщения в телеграм канал и шел на ноду искать в логе инстанс, переселение которого привело к срабатыванию алерта.

## Диагностика
- Найти в journalctl инстанс(ы) для которых был превышен порог времени миграции в 15 сек: `sudo journalctl -r | grep "Instance freeze time during migration"`
- Посмотреть в логах были ли долгие переподключения NBS или что-то помимо QEMU, что могло увелчить время миграции.
- Если были переподключения к NBS, предложить дежурному по NBS посмотреть на это.
- Cобрать данные для найденных инстансов как написано в [тикете](https://st.yandex-team.ru/CLOUD-86407) и сохранить эти данные в этом же тикете
- Дальше разбираться и анализировать собранное силами команды core

## Ссылки
[Тикет на создание этого алерта](https://st.yandex-team.ru/CLOUD-86028)
[Тикет для сбора данных о долгих миграциях](https://st.yandex-team.ru/CLOUD-86407)
