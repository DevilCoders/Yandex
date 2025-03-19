## Генератор дашбордов для Grafana

### Quick start

Для работы генератора потребуется спецификация.

#### Тестовая спецификация

1. Создадим новый файл спецификации в папке `arcadia/cloud/java/dashboard`:
   ```bash
   $ cd src/test/resources/dashboard
   $ touch myTestSpec.yaml
   ```
2. Свяжем его с дашбордом в Grafana.  
   Для этого, в созданной спецификации нужно указать поле `uid` дашборда –
   глобально уникальный идентификатор, включающий до 40 букв, цифр или символов `-`, `_`.
   Его можно:
    - задать самостоятельно (это более простой вариант; если дашборда с таким `uid` не существует,
   он будет создан автоматически);
   - либо получить,
   [создав новый дашборд вручную в UI Grafana](#создание-дашборда-вручную-в-ui-grafana).
   ```yaml
   uid: myUniqueTestUid
   ```
3. Добавим в спецификацию название:
   ```yaml
   title: My test dashboard title # должен быть уникален в рамках папки дашборда
   ```
   Теперь мы получили минимальную валидную конфигурацию дашборда.  
   ⓘ В данном случае, строка `title` не содержит спец. символов,
   поэтому, несмотря на пробелы, кавычки/апострофы необязательны.   

4. Добавим график:
   ```yaml
   graphDefaults: { datasource: 'Solomon', width: 12, height: 8 }
   queryDefaults: { labels: 'project=solomon, cluster=prestable, service=sys' }
   
   panels:
     - type: graph
       title: Demo graph
       queries:
         - params: { labels: 'host=solomon-pre-front-sas-01, path=/System/UserTime, cpu=*' }
           groupByTime: { avg: '1m' }
           select: { alias: '{{cpu}}' } # можно не задавать: линии графика и так получат название из изменяющейся метки cpu
   ```

#### Загрузка и обновление дашборда в Grafana

Для загрузки (и в дальнейшем – обновления) содержимого дашборда в Grafana, необходимо:
- [получить OAuth token](INSTALL.md#oauth-токен);
- [собрать](INSTALL.md#самостоятельная-сборка) приложение
  или [скачать](INSTALL.md#получение-готового-docker-образа) его docker-образ;
- выполнить приложение, например с помощью одной из [этих команд](INSTALL.md#примеры-запуска)
  (см. секции [CLI](INSTALL.md#cli) и [IDE](INSTALL.md#ide)),
  заменив действие `local` на `diff` (dry-run) или `upload`.  
  ⓘ Если соединение не устанавливается, попробуйте добавить к запуску `java` опцию `-Djava.net.preferIPv6Addresses=true`.  
  ⓘ Если вы получаете ошибку:
    `java.net.UnknownHostException: grafana.yandex-team.ru: Temporary failure in name resolution`,
    можете попробовать явно прописать контейнеру сеть хоста:
    `docker run --network="host" ...`.  
  ⓘ Возможные причины HTTP-ошибок описаны в
  [HTTP API Grafana](https://grafana.com/docs/grafana/latest/http_api/dashboard/).
  В частности, при `HTTP 412`, нужно проверить, нет ли в одном `folder` дашбордов с одинаковыми `uid` или `title`.

Воспользуемся готовым скриптом загрузки
[arcadia/cloud/java/dashboard/spec-single.sh](../spec-single.sh):
```bash
# проверим корректность созданной конфигурации
$ ./spec-single.sh local test myTestSpec.yaml

# если всё верно, загрузим дашборд
$ ./spec-single.sh upload test myTestSpec.yaml
```

#### Верификация

Чтобы посмотреть результат обновления дашборда, можно воспользоваться:
- ссылкой `https://grafana.yandex-team.ru/d/<uid>`
  (она же выводится в консоль генератора после обновления дашборда в Grafana);
- поиском в UI Grafana по `title` дашборда.

Перечень изменений доступен в UI Grafana в меню `⚙ → Versions → <check 2 versions> → [Compare revisions] → [View JSON Diff]`.
Этот перечень должен соответствовать diff'у, который в консоли напечатал генератор в процессе своей работы.

#### Создание дашборда вручную в UI Grafana

1. зайти в [Grafana UI](https://grafana.yandex-team.ru);
2. создать дашборд:
   -  с помощью меню слева (выбрать `[+] → Create → Dashboard`);
   - в меню справа зайти в настройки: `⚙`;
   - в меню настроек (слева) нажать `[Save]`;
3. для копирования в yaml-спецификацию, получить `uid` одним из способов:
   - скопировать из url: `https://grafana.yandex-team.ru/d/<uid>...`
   - зайти в настройки (`⚙`), выбрать в меню слева `JSON Model`, найти среди полей (они упорядочены по алфавиту) `uid`.
