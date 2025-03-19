## Порядок выкатки:

1. Порядок выкатки: Testing -> Preprod -> Prod
    * Testing: Internal-dev -> Testing
    * Preprod: Internal-Prestable -> Preprod
    * Prod: Internal-Prod -> PROD.

2. Порядок выкатки по дням
    * 1-й день: Internal-dev, Testing, Private-Testing, Preprod
    * 2-й день: Internal Prod утром ноду, после обеда весь
    * 3-й день: 1-на нода myt Prod. При выкатке **identity** перезагрузить 1-ну ноду **Access Service** в той же зоне.
    * 4-й день: Зона myt Prod. При выкатке **identity** перезагрузить ноды **Access Service** в той же зоне.
    * 5-й день: Остальные зоны Prod. При выкатке **identity** перезагрузить ноды **Access Service** в соответсвующих зонах/стендах.

{% note info %}

Для всех стендов выкатываем сначала одну ноду, смотрим, что нет ошибок, затем катим зону, потом весь стенд.

{% endnote %}

## Плановое обновление конфигурации на заданом стенде:
1. Создать [заявку на релиз](https://forms.yandex-team.ru/surveys/11002)
2. Прокатить нужные стенд.
3. Закрыть тикет

## Ad hoc обновление
**Internal Prod, Prod**
Конфигурации на определенных машинах/стенде делается через [CLOUDOPS](https://st.yandex-team.ru/createTicket?queue=CLOUDOPS)

**Остальные**
На остальных стендах изменения делаются без **CLOUDOPS**

