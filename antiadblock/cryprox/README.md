# CRYPROX

Antiadblock proxy service

1. [Описание](#описание)  
1. [Платформа](#платформа)
1. [Разработка в Аркадии](/arc/trunk/arcadia/antiadblock/cryprox/docs/Arcadia.md)
1. [Как собрать релиз](/arc/trunk/arcadia/antiadblock/cryprox/docs/Release.md)
1. [Support docs. Все что нужно знать для поддержки](/arc/trunk/arcadia/antiadblock/support)
1. [Официальная документация для партнеров](https://tech.yandex.ru/antiadblock/doc/concepts/about-docpage/)
1. [CI JOBS](#джобы)
1. [Тестирование в Sandbox](/arc/trunk/arcadia/antiadblock/cryprox/docs/Shm.md)
1. [Эксперименты во внутренней сети](/arc/trunk/arcadia/antiadblock/cryprox/docs/InternalExp.md)
1. [Мониторинги](#мониторинг)
1. [Алерты](/arc/trunk/arcadia/antiadblock/support/KB/monitoring.md)
1. [Слежение за правилами блокировщиков](#мониторинг-правил-adblock)
1. [Подключение новых партнеров](#подключение-новых-партнеров)

## Описание
Приложение состоит из:
* прокси-приложения на python. Код лежит в папке `cryprox/`, `cryprox_run/`, 
* NGINX для ускорения обработки запросов. Код лежит в папке `nginx/`

## Платформа
Краткое описание платформы

#### Прод
* **Сеть:** `ANTIADBNETS`
* **Текущая реализация:** Сервисы в Nanny (cryprox_sas, cryprox_vla, cryprox_man, cryprox_myt, cryprox_iva), аллоцированные в YP
* **L3 Балансер**: [cryprox.yandex.net](https://l3.tt.yandex-team.ru/service/4148)
* **L7 Балансер**: [AWACS Namespace cryprox.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/show/), [подробнее про балансеры](/arc/trunk/arcadia/antiadblock/cryprox/docs/L7_balancers.md)

#### Тестинг
* **Сеть:** `ANTIADBNETS`
* **Текущая реализация:** Сервисы в Nanny (test_cryprox_sas, test_cryprox_vla, test_cryprox_man), аллоцированные в YP
* **L3 Балансер**: [cryprox-test.yandex.net](https://l3.tt.yandex-team.ru/service/4149)
* **L7 Балансер**: [AWACS Namespace cryprox-test.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox-test.yandex.net/show/)

## Мониторинг
Elastic Search: https://ubergrep.yandex-team.ru/app/kibana

Solomon:
* [Весь сервис](https://solomon.yandex-team.ru/?project=Antiadblock&dashboard=Antiadblock_Overall)
* [Partner](https://solomon.yandex-team.ru/?project=Antiadblock&cluster=cryprox-prod&service=cryprox_actions&service_id=autoru&dashboard=Antiadblock_system_partner_dashboard)

## Джобы
* Сборка Docker образов и тестирование:    
[джоба в testenv](https://testenv.yandex-team.ru/?screen=job_history&database=antiadblock&job_name=BUILD_ANTIADBLOCK_UNI_PROXY_DOCKER)

## Мониторинг правил adblock
Мониторинг правил в adblock листах: [sonar](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/adblock_rule_sonar/README.md)

## Подключение новых партнеров
Для начала процедуры подключения нового партнера необходимо:

* Выдать партнеру [документацию](https://tech.yandex.ru/antiadblock/doc/concepts/about-docpage/)
* Получить от партнера заполненную анкету/ответы на вопросы из инструкции для партнеров
* На базе полученной информации создать сервис в админке

Далее начинается тестирование и пробное подключение с партнером.
