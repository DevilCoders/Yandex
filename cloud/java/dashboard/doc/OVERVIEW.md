## Генератор дашбордов для Grafana 

### Overview

Генератор представляет собой standalone java-приложение, запускаемое напрямую или с помощью docker.
Схема его работы:

    [spec.yaml] ⟹ [[dashboard generator]] ⟺ [dashboard.json] ⟺ [[Grafana]]

* входная спецификация, представляющая собой yaml-файл, поступает в генератор;
* генератор преобразует её в json, описывающий дашборд в формате [Grafana](https://grafana.yandex-team.ru),
  с поддержкой запросов в datasource [Solomon](https://solomon.yandex-team.ru);
* полученный дашборд сравнивается с текущей версией в Grafana;
  при необходимости, осуществляется обновление remote-версии в Grafana.  

#### Примеры

- demo-дашборд:
  [спецификация](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/resources/dashboard/demo.yaml) (yaml),
  [результат](https://grafana.yandex-team.ru/d/XlF7fo_iz?editview=dashboard_json) (json),
  [дашборд](https://grafana.yandex-team.ru/d/XlF7fo_iz);
- дашборд – "entry point" для дашбордов службы:
  [спецификация](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/main.yaml) (yaml),
  [результат](https://grafana.yandex-team.ru/d/ycpMain?editview=dashboard_json) (json),
  [дашборд](https://grafana.yandex-team.ru/d/ycpMain); 
- дашборд дежурного platform team:
  [спецификация](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/duty/duty.yaml) (yaml),
  [результат](https://grafana.yandex-team.ru/d/ycpDuty?editview=dashboard_json) (json),
  [дашборд](https://grafana.yandex-team.ru/d/ycpDuty).

#### Мотивация

По сравнению с ручным вводом графиков, подход, основанный на хранимых в VCS спецификациях, имеет значительные плюсы, в числе которых:
- спецификация как код; 
- её распределённое хранение – в репозитории и на рабочих станциях – вместе с кодом, формирующим метрики для неё;
- версионирование и аудит – не только дашбордов, но и их спецификаций;
- консистентность: все графики используют одинаковый подход к решению аналогичных задач;
- уровень абстракции: можно управлять форматом, значениями по умолчанию и т.п.; 
- отсутствие legacy-артефактов, накопленных за время жизни и поддержки графика;
- лёгкость внесения общих изменений на все графики одновременно;
- лёгкость синхронизации всех изменённых дашбордов "по кнопке", включая иерархию drilldown-дашбордов;
- лёгкость повторного использования отдельных частей дашбордов; 
- (Grafana plugin specific) возможность использования более сложных выражений, чем позволяет отобразить UI
  (ввиду отсутствия необходимости их редактирования с помощью UI).
