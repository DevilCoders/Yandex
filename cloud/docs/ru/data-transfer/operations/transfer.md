# Управление трансфером

Вы можете:

* [получить список трансферов](#list);
* [создать трансфер](#create);
* [изменить трансфер](#update);
* [активировать трансфер](#activate);
* [деактивировать трансфер](#deactivate);
* [перезагрузить трансфер](#reupload);
* [удалить трансфер](#delete).

Подробнее о состояниях трансфера, возможных действиях с ним и имеющихся ограничениях см. в разделе [{#T}](../concepts/transfer-lifecycle.md).

## Получить список трансферов {#list}

{% list tabs %}

- Консоль управления

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.

{% endlist %}

## Создать трансфер {#create}

{% list tabs %}

- Консоль управления

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.
    1. Нажмите кнопку **Создать трансфер**.
    1. Выберите эндпоинт для источника или [создайте](./endpoint/index.md#create) новый.
    1. Выберите эндпоинт для приемника или [создайте](./endpoint/index.md#create) новый.
    1. Укажите параметры трансфера:

        * Имя трансфера.
        * (Опционально) Описание трансфера.
        * Тип трансфера:

            * `{{ dt-type-copy-repl }}` — чтобы создать полную копию данных источника и поддерживать ее в актуальном состоянии.
            * `{{ dt-type-copy }}` — чтобы создать полную копию данных без дальнейшего получения обновлений из источника.
            * `{{ dt-type-repl }}` — чтобы непрерывно получать изменения данных от источника и применять их к приемнику (без создания полной копии данных источника).

        * (Опционально) Среда выполнения — тип системы и параметры запуска трансфера:

            * {{ yandex-cloud }}:

                * Параметры шардированного копирования — параметры параллельного чтения из таблиц для увеличения пропускной способности трансфера. Параллельность доступна только для таблиц, которые содержат первичный ключ в [режиме serial](https://www.postgresql.org/docs/current/datatype-numeric.html#DATATYPE-SERIAL).

                    * Количество инстансов — количество виртуальных машин в {{ compute-full-name }}, на которых будет запущен трансфер. Рекомендуется указывать значение от 2 до 8.
                    * Количество процессов — количество потоков для выполнения трансфера в инстансе. Рекомендуется указывать значение от 4 до 8.

        * (Опционально) Список объектов для переноса — будут передаваться только объекты из этого списка. Если в настройках эндпоинта-источника указан список включенных таблиц или коллекций, передаваться будут только те объекты, которые есть в обоих списках.

            Укажите полное имя объекта. В зависимости от типа источника используйте соответствующую схему именования:
​
            * {{ CH }} — `<имя базы>.<имя таблицы>`;
            * {{ GP }} — `<имя базы>.<имя схемы>.<имя таблицы>`;
            * {{ MG }} — `<имя базы>.<имя коллекции>`;
            * {{ MY }} — `<имя базы>.<имя таблицы>`;
            * {{ PG }} — `<имя базы>.<имя схемы>.<имя таблицы>`.

            Если указанный объект находится в списке исключенных таблиц или коллекций в настройках эндпоинта-источника, или имя объекта введено некорректно, трансфер завершится с ошибкой. Работающий трансфер в статусе **{{ dt-type-repl }}** или **{{ dt-type-copy-repl }}** завершится сразу, незапущенный трансфер — в момент активации.

    1. Нажмите кнопку **Создать**.

- {{ TF }}

    {% include [terraform-definition](../../_tutorials/terraform-definition.md) %}

    Чтобы создать трансфер:

    1. В командной строке перейдите в каталог, в котором будут расположены конфигурационные файлы {{ TF }} с планом инфраструктуры. Если такой директории нет — создайте ее.

    {% if audience != "internal" %}
    1. Если у вас еще нет {{ TF }}, [установите его и создайте конфигурационный файл с настройками провайдера](../../tutorials/infrastructure-management/terraform-quickstart.md#install-terraform).
    {% endif %}
    1. Создайте конфигурационный файл с описанием трансфера.

       Пример структуры конфигурационного файла:

       ```hcl
       resource "yandex_datatransfer_transfer" "<имя трансфера в {{ TF }}>" {
         folder_id   = "<идентификатор каталога>"
         name        = "<имя трансфера>"
         description = "<описание трансфера>"
         source_id   = "<идентификатор эндпоинта-источника>"
         target_id   = "<идентификатор эндпоинта-приемника>"
         type        = "<тип трансфера>"
       }
       ```

       Доступные типы трансферов:

       * `SNAPSHOT_ONLY` — _{{ dt-type-copy }}_;
       * `INCREMENT_ONLY` — _{{ dt-type-repl }}_;
       * `SNAPSHOT_AND_INCREMENT` — _{{ dt-type-copy-repl }}_.

    1. Проверьте корректность настроек.

        {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

    1. Подтвердите изменение ресурсов.

       {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

    Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-dt-transfer }}).    

    При создании трансферы типа `INCREMENT_ONLY` и `SNAPSHOT_AND_INCREMENT` активируются и запускаются автоматически.
    Если вы хотите активировать трансфер типа `SNAPSHOT_ONLY` в момент его создания, то добавьте в конфигурационный файл секцию `provisioner "local-exec"` с командой активации трансфера:

    ```hcl
       provisioner "local-exec" {
          command = "yc --profile <профиль> datatransfer transfer activate ${yandex_datatransfer_transfer.<имя Terraform-ресурса трансфера>.id
       }
    ``` 
    
    В этом случае копирование выполнится только один раз в момент создания трансфера.
    
    

{% endlist %}

## Изменить трансфер {#update}

{% list tabs %}

- Консоль управления

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.
    1. Выберите трансфер и нажмите кнопку ![pencil](../../_assets/pencil.svg) **Редактировать** на панели сверху.
    1. Измените параметры трансфера:

        * Имя трансфера.
        * Описание трансфера.
        * Среда выполнения — тип системы и параметры запуска трансфера:

            * {{ yandex-cloud }}:

                * Параметры шардированного копирования — параметры параллельного чтения из таблиц для увеличения пропускной способности трансфера. Параллельность доступна только для таблиц, которые содержат первичный ключ в [режиме serial](https://www.postgresql.org/docs/current/datatype-numeric.html#DATATYPE-SERIAL).

                    * Количество инстансов — количество виртуальных машин в {{ compute-full-name }}, на которых будет запущен трансфер. Рекомендуется указывать значение от 2 до 8.
                    * Количество процессов — количество потоков для выполнения трансфера в инстансе. Рекомендуется указывать значение от 4 до 8.

        * Список объектов для переноса — будут передаваться только объекты из этого списка. Если в настройках эндпоинта-источника указан список включенных таблиц или коллекций, передаваться будут только те объекты, которые есть в обоих списках.

            Укажите полное имя объекта. В зависимости от типа источника используйте соответствующую схему именования:
​
            * {{ CH }} — `<имя базы>.<имя таблицы>`;
            * {{ GP }} — `<имя базы>.<имя схемы>.<имя таблицы>`;
            * {{ MG }} — `<имя базы>.<имя коллекции>`;
            * {{ MY }} — `<имя базы>.<имя таблицы>`;
            * {{ PG }} — `<имя базы>.<имя схемы>.<имя таблицы>`.

            Если указанный объект находится в списке исключенных таблиц или коллекций в настройках эндпоинта-источника, или имя объекта введено некорректно, трансфер завершится с ошибкой. Работающий трансфер в статусе **{{ dt-type-repl }}** или **{{ dt-type-copy-repl }}** завершится сразу, незапущенный трансфер — в момент активации.

    1. Нажмите кнопку **Сохранить**.

- {{ TF }}

  1. Откройте актуальный конфигурационный файл {{ TF }} с описанием трансфера.

      О том, как создать такой файл, см. в подразделе [Создать трансфер](#create).

  1. Измените значение полей `name` и `description` (имя и описание трансфера).
  1. Проверьте корректность настроек.

      {% include [terraform-validate](../../_includes/mdb/terraform/validate.md) %}

  1. Подтвердите изменение ресурсов.

      {% include [terraform-apply](../../_includes/mdb/terraform/apply.md) %}

  Подробнее см. в [документации провайдера {{ TF }}]({{ tf-provider-dt-transfer }}).

{% endlist %}

## Активировать трансфер {#activate}

{% list tabs %}

- Консоль управления

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.
    1. Нажмите на значок ![ellipsis](../../_assets/horizontal-ellipsis.svg) рядом с именем нужного трансфера и выберите пункт **Активировать**.

{% endlist %}

## Перезагрузить трансфер {#reupload}

Если вы предполагаете, что этап репликации трансфера может завершиться ошибкой (например, из-за [изменения схемы переносимых данных](db-actions.md) на источнике), принудительно перезагрузите трансфер.

{% list tabs %}

- Консоль управления

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.
    1. Нажмите на значок ![ellipsis](../../_assets/horizontal-ellipsis.svg) рядом с именем нужного трансфера и выберите пункт **Перезагрузить**.

{% endlist %}

Подробнее см. в разделе [{#T}](../concepts/transfer-lifecycle.md).

## Деактивировать трансфер {#deactivate}

В процессе деактивации трансфера:

* отключается слот репликации на источнике;
* удаляются временные логи переноса данных;
* приемник приводится в согласованное состояние:
    * переносятся объекты схемы данных источника для финальной стадии;
    * создаются индексы.

{% list tabs %}

- Консоль управления

    1. Переведите источник в режим <q>только чтение</q> (read-only).
    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.
    1. Нажмите на значок ![ellipsis](../../_assets/horizontal-ellipsis.svg) рядом с именем нужного трансфера и выберите пункт **Деактивировать**.
    1. Дождитесь перехода трансфера в статус {{ dt-status-stopped }}.

{% endlist %}

{% note warning %}

Не прерывайте деактивацию трансфера! Если процесс завершится некорректно, работоспособность источника и приемника не гарантируется.

{% endnote %}

Подробнее см. в разделе [{#T}](../concepts/transfer-lifecycle.md).

## Удалить трансфер {#delete}

{% list tabs %}

- Консоль управления

    1. Перейдите на [страницу каталога]({{ link-console-main }}) и выберите сервис **{{ data-transfer-full-name }}**.
    1. На панели слева выберите ![image](../../_assets/data-transfer/transfer.svg) **Трансферы**.
    1. Если нужный трансфер находится в активном состоянии, [деактивируйте его](#deactivate).
    1. Нажмите на значок ![ellipsis](../../_assets/horizontal-ellipsis.svg) рядом с именем нужного трансфера и выберите пункт **Удалить**.
    1. Нажмите кнопку **Удалить**.

- {{ TF }}

    {% include [terraform-delete](../../_includes/data-transfer/terraform-delete-transfer.md) %}

{% endlist %}
