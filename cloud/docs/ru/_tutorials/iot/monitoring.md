# Мониторинг показаний датчиков и уведомления о событиях

В этом сценарии вы настроите мониторинг и уведомления об изменениях для показаний датчиков, подключенных к сервису {{ iot-full-name }}. Датчики будут эмулированы с помощью сервиса {{ sf-full-name }}. Если у вас есть подключенные датчики, используйте их.

Для работы со сценарием вам не нужно создавать и настраивать виртуальные машины — вся работа основана на бессерверных вычислениях {{ sf-name }}. Исходный код, который используется в этом сценарии, доступен на [GitHub](https://github.com/yandex-cloud/examples/tree/master/iot/Scenarios/ServerRoomMonitoring).

Чтобы настроить мониторинг показаний датчиков в серверной комнате:
1. [Подготовьте облако к работе](#before-you-begin)
{% if product == "yandex-cloud" %}1. [Необходимые платные ресурсы](#paid-resources){% endif %}
1. [Создайте необходимые ресурсы {{ iot-full-name }}](#resources-step)
    1. [Создайте реестр](#registry-step)
    1. [Создайте устройство](#device-step)
1. [Создайте эмулятор устройства на базе {{ sf-full-name }}](#emulator-step)
    1. [Создайте функцию эмуляции отправки данных с устройства](#emulation-function)
    1. [Протестируйте функцию эмуляции отправки данных](#test-emulation-function)
    1. [Создайте триггер вызова функции эмуляции один раз в минуту](#minute-trigger)
    1. [Создайте функцию обработки принимаемых данных](#processing-function)
    1. [Протестируйте функцию обработки данных](#test-processing-function)
    1. [Создайте триггер вызова функции обработки данных по сигналу](#signal-trigger)
1. [Настройте мониторинг показаний датчиков](#configure-monitoring)
    1. [Создайте графики](#create-widgets)
    1. [Создайте дашборд](#create-dashboard)
    1. [Протестируйте работу графиков на дашборде](#test-dashboard)
    1. [Создайте алерты](#create-alerts)
1. [Удалите созданные облачные ресурсы](#cleanup)

## Подготовьте облако к работе {#before-you-begin}

{% include [before-you-begin](../_tutorials_includes/before-you-begin.md) %}

Если у вас еще нет интерфейса командной строки {{ yandex-cloud }}, [установите и инициализируйте его](../../cli/quickstart.md#install).

{% if product == "yandex-cloud" %}

### Необходимые платные ресурсы {#paid-resources}

В стоимость входят:
* плата за количество сообщений сервиса {{ iot-full-name }} (см. [тарифы](../../iot-core/pricing.md));
* плата за количество вызовов функции сервиса {{ sf-full-name }} (см. [тарифы](../../functions/pricing.md));
* плата за запись пользовательских метрик через API сервиса {{ monitoring-full-name }} {% if audience == external %}(см. [тарифы](../../monitoring/pricing.md)){% endif %}.

{% endif %}

## Создайте необходимые ресурсы {{ iot-short-name }} {#resources-step}

[Реестр](../../iot-core/concepts/index.md#registry) и [устройство](../../iot-core/concepts/index.md#device) — основные компоненты сервиса {{ iot-short-name }} для обмена данными и командами. Чтобы устройства могли обмениваться данными, их необходимо создавать в одном реестре.

### Создайте реестр и настройте авторизацию по логину и паролю {#registry-step}

Чтобы создать реестр:  
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ iot-short-name }}**.
1. Нажмите кнопку **Создать реестр**.
1. В поле **Имя** введите имя реестра. Например, `my-registry`.
1. В поле **Пароль** задайте пароль доступа к реестру.  
   
    Для создания пароля можно воспользоваться [генератором паролей](https://passwordsgenerator.net/).  
    Не забудьте сохранить пароль, он вам понадобится.
1. (опционально) В поле **Описание** добавьте дополнительную информацию о реестре.
1. Нажмите кнопку **Создать**.

Вы также можете использовать авторизацию с помощью сертификатов. Подробнее [об авторизации в {{ iot-short-name }}](../../iot-core/concepts/authorization.md).

### Создайте устройство и настройте авторизацию по логину и паролю {#device-step}

Чтобы создать устройство:  
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ iot-short-name }}**.
1. Выберите реестр, созданный на предыдущем шаге.
1. В левой части окна выберите раздел **Устройства**.
1. Нажмите кнопку **Добавить устройство**.
1. В поле **Имя** введите имя устройства. Например, `my-device`.
1. В поле **Пароль** задайте пароль доступа к устройству.  
    
    Для создания пароля можно воспользоваться [генератором паролей](https://passwordsgenerator.net/).  
    Не забудьте сохранить пароль, он вам понадобится.  
1. (опционально) В поле **Описание** добавьте дополнительную информацию об устройстве.
1. (опционально) Добавьте алиас:
    1. Нажмите кнопку **Добавить алиас**.
    1. Заполните поля: введите алиас (например `events`) и тип топика после `$devices/<deviceID>` (например `events`).  
        
        Вы сможете использовать алиас `events` вместо топика `$devices/<deviceID>/events`.
    1. Повторите действия для каждого добавляемого алиаса.
1. Нажмите кнопку **Создать**.
1. Повторите действия для каждого устройства, которое вы хотите создать.

Вы также можете использовать авторизацию с помощью сертификатов. Подробнее [об авторизации в {{ iot-short-name }}](../../iot-core/concepts/authorization.md).

## Создайте эмулятор устройства в {{ sf-name }} {#emulator-step}

Эмулятор отправляет данные с датчиков устройства и обрабатывает данные для мониторинга и алертов.

Вам потребуется:
* создать и протестировать [функцию](../../functions/concepts/function.md) эмуляции отправки данных с датчиков каждого устройства;
* создать [триггер](../../functions/concepts/trigger/index.md) вызова функции эмуляции один раз в минуту;
* создать и протестировать функцию обработки принимаемых данных;
* создать триггер вызова функции обработки данных по сигналу.  

### Создайте функцию эмуляции отправки данных с устройства {#emulation_function}

Чтобы создать функцию:  
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. В левой части окна выберите раздел **Функции**.
1. Нажмите кнопку **Создать функцию**.
1. В поле **Имя** введите имя функции. Например, `my-device-emulator-function`.
1. (опционально) В поле **Описание** добавьте дополнительную информацию о функции.
1. Нажмите кнопку **Создать**.
1. В открывшемся окне **Редактор** в списке **Среда выполнения** выберите `nodejs12`.
1. Выберите **Способ**: **Редактор кода**.
1. В левой части окна **Редактор кода** нажмите кнопку **Создать файл**.
1. В открывшемся окне **Новый файл** ведите имя файла `device-emulator.js`.
1. Нажмите кнопку **Создать**.
1. Выберите созданный файл в левой части окна **Редактор кода**.
1. В правой части окна **Редактор кода** вставьте код функции с [GitHub](https://github.com/yandex-cloud/examples/blob/master/iot/Scenarios/ServerRoomMonitoring/device-emulator.js).
1. В поле **Точка входа** введите `device-emulator.handler`.
1. В поле **Таймаут, с** введите `10`.
1. В поле **Память** оставьте значение `128 МБ`.
1. Создайте сервисный аккаунт, от имени которого функция отправит данные в {{ iot-short-name }}:
    1. Нажмите кнопку **Создать аккаунт**.
    1. В открывшемся окне **Создание сервисного аккаунта** в поле **Имя** введите имя аккаунта. Например, `my-emulator-function-service-account`.
    1. Добавьте роли для вызова функции и записи в ресурсы `serverless.functions.invoker` и `iot.devices.writer`:
        1. Нажмите на значок ![image](../../_assets/plus-sign.svg).
        1. Выберите роль в списке.
        1. Нажмите кнопку **Создать**.
1. Настройте параметр **Переменные окружения** для каждого датчика серверной комнаты:
    1. Нажмите кнопку **Добавить переменную окружения**.
    1. Заполните поля **Ключ** и **Значение** для переменных окружения:
        
        Ключ | Описание | Значение
        :----- | :----- | :-----
        `HUMIDITY_SENSOR_VALUE` | Базовое значение показания датчика влажности. | `80.15`
        `TEMPERATURE_SENSOR_VALUE` | Базовое значение показания датчика температуры. | `25.25`
        `RACK_DOOR_SENSOR_VALUE` | Показания датчика открытия дверцы стойки. | `False`
        `ROOM_DOOR_SENSOR_VALUE` | Показания датчика открытия двери в серверную комнату. | `False`
        `SMOKE_SENSOR_VALUE` | Показания детектора дыма. | `False`
        `WATER_SENSOR_VALUE` | Показания детектора воды. | `False`
        `IOT_CORE_DEVICE_ID` | Идентификатор устройства, которое вы создали. | См. в консоли управления <br>сервиса **{{ iot-short-name }}**.
        `DEVICE_ID` | Пользовательское название устройства. | Задается пользователем.
1. В правой верхней части окна нажмите кнопку **Создать версию**.

### Протестируйте функцию эмуляции {#test-emulation-function}

Чтобы протестировать функцию:
1. (опционально) Для получения подробной информации с датчиков, подпишите реестр на топик устройства {{ iot-full-name }}, где`$devices/<deviceID>/events` — топик устройства, `<deviceID>` — ID устройства в сервисе:

    {% list tabs %}
    
    - CLI  
        
        {% include [cli-install](../../_includes/cli-install.md) %}
        
        Выполните команду: 
    
        ```
        yc iot mqtt subscribe \
              --username <ID реестра> \
              --password <пароль реестра> \
              --topic '$devices/<ID устройства>/events' \
              --qos 1
        ```

        Где:

        * `--username` и `--password` — параметры авторизации с помощью логина и пароля.
        * `--topic` — топик устройства для отправки данных.
        * `--message` — текст сообщения.
        * `--qos` — уровень качества обслуживания (QoS).
    
    {% endlist %}
	
   Подробнее о [подписке на топики устройства в {{ iot-full-name }}](../../iot-core/operations/subscribe#one-device).
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. В левой части окна выберите раздел **Тестирование**.
1. В списке **Тег версии** выберите `$latest` — последнюю созданную функцию.
1. Нажмите кнопку **Запустить тест**.

При успешном выполнении функции в поле **Состояние функции** отобразится статус **Выполнена** и в поле **Ответ функции** появится результат:

```
{
"statusCode" : 200
}
```

Если вы подписались на топик устройства {{ iot-short-name }}, вы получите JSON вида:

```json
{
"":"0e3ce1d0-1504-4325-972f-55c961319814",
"TimeStamp":"2020-05-21T22:38:12Z",
"Values":[
    {"Type":"Float","Name":"Humidity","Value":"25.33"},
    {"Type":"Float","Name":"Temperature","Value":"80.90"},
    {"Type":"Bool","Name":"Water sensor","Value":"False"},
    {"Type":"Bool","Name":"Smoke sensor","Value":"False"},
    {"Type":"Bool","Name":"Room door sensor","Value":"False"},
    {"Type":"Bool","Name":"Rack door sensor","Value":"False"}
    ]
}
```

Подробнее об [MQTT-топиках в сервисе {{ iot-short-name }}](../../iot-core/concepts/topic/index.md).

### Создайте триггер вызова функции один раз в минуту {#minute-trigger}

Чтобы создать триггер:
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. Выберите раздел **Триггеры**.
1. Нажмите кнопку **Создать триггер**.
1. В поле **Имя** введите имя триггера. Например, `my-emulator-function-trigger`.
1. (опционально) В поле **Описание** добавьте дополнительную информацию о триггере.
1. Выберите **Тип**: **Таймер**.
1. В поле **Cron-выражение** введите `* * * * ? *` (вызов один раз в минуту).
1. В блоке **Настройки функции** введите ранее заданные параметры функции:
    * **Функция**: `my-device-emulator-function`.
    * **Тег версии функции**: `$latest`.
    * **Сервисный аккаунт**: `my-emulator-function-service-account`.
1. (опционально) Настройте параметры блоков **Настройки повторных запросов** и **Настройки Dead Letter Queue**. Они обеспечивают сохранность данных.
    * **Настройки повторных запросов** позволяют повторно вызывать функцию, если текущий вызов функции завершается с ошибкой.
    * **Настройки Dead Letter Queue** позволяют перенаправлять сообщения, которые не смогли обработать получатели в обычных очередях. 
        В качестве DLQ очереди вы можете настроить стандартную очередь сообщений. Если вы еще не создавали очередь сообщений, [создайте ее в сервисе {{ message-queue-full-name }}](../../message-queue/operations/message-queue-new-queue.md).
1. Нажмите кнопку **Создать триггер**.

### Создайте функцию обработки принимаемых данных {#processing_function}

Чтобы создать функцию:
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. В левой части окна выберите раздел **Функции**.
1. Нажмите кнопку **Создать функцию**.
1. В поле **Имя** введите имя функции. Например, `my-monitoring-func`.
1. (опционально) В поле **Описание** добавьте дополнительную информацию о функции.
1. Нажмите кнопку **Создать**.
1. В открывшемся окне **Редактор** в списке **Среда выполнения** выберите `python37`.
1. Выберите **Способ**: нажмите на вкладку **Редактор кода**.
1. В левой части окна **Редактор кода** нажмите кнопку **Создать файл**.
1. В открывшемся окне **Новый файл** ведите имя файла `monitoring.py`.
1. Нажмите кнопку **Создать**.
1. В левой части окна **Редактор кода** выберите созданный файл.
1. В правой части окна вставьте код функции с [GitHub](https://github.com/yandex-cloud/examples/blob/master/iot/Scenarios/ServerRoomMonitoring/monitoring.py).
    
    В этой функции подготовка данных для отправки в сервис мониторинга находится в методе `makeAllMetrics`. Если вы захотите добавить или удалить параметры, выполните изменения в этом методе.
1. В поле **Точка входа** окна **Редактор** введите `monitoring.msgHandler`.
1. В поле **Таймаут, с** введите `10`.
1. В поле **Память** оставьте значение `128 МБ`.
1. Создайте сервисный аккаунт, от имени которого функция обработает данные, полученные от устройства:
    1. Нажмите кнопку **Создать аккаунт**.
    1. В открывшемся окне **Создание сервисного аккаунта** в поле **Имя** введите имя аккаунта. Например, `my-metrics-function-service-account`.
    1. Добавьте роли вызова функции и изменения ресурсов `serverless.functions.invoker` и `editor`:
        1. Нажмите на значок ![image](../../_assets/plus-sign.svg).
        1. Выберите роль в списке.
        1. Нажмите кнопку **Создать**.
    1. Настройте параметр **Переменные окружения**:
        1. Нажмите кнопку **Добавить переменную окружения**.   
        1. Заполните поля **Ключ** и **Значение** для переменных окружения:
            Ключ | Описание | Значение
            :----- | :----- | :-----
            `VERBOSE_LOG` | Включение и отключение записи данных. | `True`
            `METRICS_FOLDER_ID` | Идентификатор каталога, в котором развернуты сервисы и для которого вы создадите дашборд в сервисе {{ monitoring-full-name }}. | См. в консоли управления.
1. В правой верхней части окна нажмите кнопку **Создать версию**.

### Протестируйте функцию обработки данных {#test-processing-function}

Чтобы протестировать функцию:
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. В левой части окна выберите раздел **Тестирование**.
1. В списке **Тег версии** выберите `$latest` — последнюю созданную функцию.
1. В поле **Входные данные** вставьте данные:

    ```json
    {
        "messages": [
            {
                "event_metadata": {
                    "event_id": "160d239876d9714800",
                    "event_type": "yandex.cloud.events.iot.IoTMessage",
                    "created_at": "2020-05-08T19:16:21.267616072Z",
                    "folder_id": "b112345678910"
                },
                "details": {
                    "registry_id": "are1234567890",
                    "device_id": "are0987654321",
                    "mqtt_topic": "$devices/are0987654321/events",
                    "payload": "eyJWYWx1ZXMiOiBbeyJUeXBlIjogIkZsb2F0IiwgIlZhbHVlIjogIjI1Ljc0IiwgIk5hbWUiOiAiSHVtaWRpdHkifSwgeyJUeXBlIjogIkZsb2F0IiwgIlZhbHVlIjogIjgwLjY1IiwgIk5hbWUiOiAiVGVtcGVyYXR1cmUifSwgeyJUeXBlIjogIkJvb2wiLCAiVmFsdWUiOiAiRmFsc2UiLCAiTmFtZSI6ICJXYXRlciBzZW5zb3IifSwgeyJUeXBlIjogIkJvb2wiLCAiVmFsdWUiOiAiRmFsc2UiLCAiTmFtZSI6ICJTbW9rZSBzZW5zb3IifSwgeyJUeXBlIjogIkJvb2wiLCAiVmFsdWUiOiAiRmFsc2UiLCAiTmFtZSI6ICJSb29tIGRvb3Igc2Vuc29yIn0sIHsiVHlwZSI6ICJCb29sIiwgIlZhbHVlIjogIkZhbHNlIiwgIk5hbWUiOiAiUmFjayBkb29yIHNlbnNvciJ9XSwgIlRpbWVTdGFtcCI6ICIyMDIwLTA1LTIxVDIzOjEwOjE2WiIsICJEZXZpY2VJZCI6ICIwZTNjZTFkMC0xNTA0LTQzMjUtOTcyZi01NWM5NjEzMTk4MTQifQ=="
                }
            }
        ]
    }
    ```
1. Нажмите кнопку **Запустить тест**.

При успешном выполнении функции в поле **Состояние функции** отобразится статус **Выполнена**, а в поле **Ответ функции** появится результат:

```json
{
"statusCode" : 200 ,
    "headers" : {
        "Content-Type" : "text/plain"
    },
"isBase64Encoded" : false
}
```

### Создайте триггер вызова функции обработки данных по сигналу {#signal-trigger}

Триггер вызовет функцию, когда в [топике устройства](../../iot-core/concepts/topic/devices-topic.md) появится сообщение.

Чтобы создать триггер:
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. Выберите раздел **Триггеры**.
1. Нажмите кнопку **Создать триггер**.
1. В поле **Имя** введите имя триггера. Например, `my-monitoring-func-trigger`.
1. (опционально) В поле **Описание** добавьте дополнительную информацию о триггере.
1. Выберите **Тип**: **{{ iot-name }}**.
1. В блоке **Настройки сообщений {{ iot-name }}** введите ранее заданные параметры реестра и устройства:
    * **Реестр**: `my-registry`.
    * **Устройство**: `my-device`.
    * **MQTT-топик**: `$devices/<deviceID>/events`, где `<deviceID>` — это ID устройства в сервисе **{{ iot-short-name }}**.
1. В блоке **Настройки функции** введите ранее заданные параметры функции:
    * **Функция**: `my-monitoring-func`.
    * **Тег версии функции**: `$latest`.
    * **Сервисный аккаунт**: `my-metrics-function-service-account`.
1. (опционально) Настройте параметры блоков **Настройки повторных запросов** и **Настройки Dead Letter Queue**. Они обеспечивают сохранность данных.
    * **Настройки повторных запросов** позволяют повторно вызывать функцию, если текущий вызов функции завершается с ошибкой.
    * **Настройки Dead Letter Queue** позволяют перенаправлять сообщения, которые не смогли обработать получатели в обычных очередях. 
        В качестве DLQ очереди вы можете настроить стандартную очередь сообщений. Если вы еще не создавали очередь сообщений, [создайте ее в сервисе {{ message-queue-full-name }}](../../message-queue/operations/message-queue-new-queue.md).
1. Нажмите кнопку **Создать триггер**.

Все данные от устройства автоматически попадут в сервис **{{ monitoring-name }}**.

## Настройте мониторинг показаний датчиков {#configure-monitoring}

Для наблюдения за показаниями датчиков используется дашборд в сервисе {{ monitoring-full-name }}. Показания датчиков поступают с контроллера на сервер раз в минуту по MQTT-протоколу. Когда показания датчиков достигают заданных значений, {{ monitoring-full-name }} отправляет уведомления пользователям. 

{% cut "Формат передачи данных"%}

```json
 {
    "DeviceId":"e7a68b2d-464e-4222-88bd-c9e8d10a70cd",
    "TimeStamp":"2020-05-21T10:16:43Z",
    "Values":[
        {"Type":"Float","Name":"Humidity","Value":"12.456"},
        {"Type":"Float","Name":"Temperature","Value":"-23.456"},
        {"Type":"Bool","Name":"Water sensor","Value":"false"},
        {"Type":"Bool","Name":"Smoke sensor","Value":"false"},
        {"Type":"Bool","Name":"Room door sensor","Value":"true"},
        {"Type":"Bool","Name":"Rack door sensor","Value":"false"}
    ]
 }
```

{% endcut %}

Настройте мониторинг показаний датчиков: создайте графики на дашбордах и алерты.

### Создайте графики {#create-diagrams}

Чтобы создать графики на дашборде:
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ monitoring-short-name }}**.
1. Перейдите на вкладку **Дашборды**.
1. Нажмите кнопку **Создать**.
1. В блоке **Добавить виджет** нажмите **Новый график**.
1. В списке сервисов **service=** выберите **Custom**.
1. В списке типов графиков **name=** выберите **Temperature**.
1. В списке **device_id=** выберите идентификатор устройства, по которому вы хотите создать график.
1. Нажмите кнопку **Сохранить**.
1. Повторите действия для каждого графика из списка:
    * `Temperature` — температуры в помещении.
    * `Humidity` — влажности в помещении.
    * `Water sensor` — воды на полу (есть вода / нет воды).
    * `Smoke sensor` — дыма (есть дым / нет дыма).
    * `Room door sensor` — открытия двери в помещение (дверь открыта / дверь закрыта).
    * `Rack door sensor` — открытия дверцы серверной стойки (дверца открыта / дверца закрыта).
1. Нажмите кнопку **Сохранить** и сохраните дашборд.
1. В открывшемся окне введите имя дашборда и нажмите кнопку **Сохранить**.
    
Дашборд доступен по ссылке всем пользователям {{ yandex-cloud }} с ролью `viewer`. Вы можете его настраивать, редактировать, менять масштаб, включать и отключать автоматическое обновление данных. {% if audience == "external" %} Подробнее о работе с [дашбордами](../../monitoring/operations/dashboard/create.md). {% endif %}
 
### Протестируйте работу графиков на дашборде {#test-dashboard}

Если поменять базовые значения в переменных окружения функции эмулирующего устройства, эти изменения отразятся на графиках.

Чтобы протестировать работу графиков:
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ sf-name }}**.
1. В левой части окна выберите раздел **Функции**, в списке функций выберите `my-device-emulator-function`.
1. Нажмите на вкладку **Редактор**.
1. В нижней части окна в блоке **Переменные окружения** в поле **Значение** замените несколько исходных значений переменных на любые другие.

    Ключ | Исходное значение | Новое значение
    :----- | :----- | :-----
    `HUMIDITY_SENSOR_VALUE` | `80.15` | `40`
    `TEMPERATURE_SENSOR_VALUE` | `25.25` | `15`
    `RACK_DOOR_SENSOR_VALUE` | `False` | `True`
    `ROOM_DOOR_SENSOR_VALUE` | `False` | `True`
    `SMOKE_SENSOR_VALUE` | `False` | `True`
    `WATER_SENSOR_VALUE` | `False` | `True`
1. Нажмите кнопку **Создать версию** в верхней части окна.
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ monitoring-short-name }}** и посмотрите, как изменились показатели графиков.

### Создайте алерт {#create-alerts}

Создайте алерт по показаниям датчика температуры в помещении и настройте список получателей алерта.  

Сервис отправит этот алерт получателям, если в течение определенного периода (`5 минут`) датчик температуры в серверной комнате будет показывать определенную температуру:
* `50 градусов`  — алерт `Warning` (предупреждение). 
* `70 градусов`  — алерт `Alarm` (критическое значение).  

Чтобы создать алерт:  
1. В [консоли управления]({{ link-console-main }}) выберите каталог, в котором вы выполняете сценарий.
1. Выберите сервис **{{ monitoring-short-name }}**.
1. Нажмите кнопку **Создать алерт**.  
1. В блоке **Основные** в поле **Имя** введите имя алерта.
1. В блоке **Метрики** в разделе **Метрики** нажмите на значок ![image](../../_assets/plus-sign.svg) и заполните поля:
    1. В списке сервисов **service=** выберите **Custom**.
    1. В списке типов алертов **name=** выберите **Temperature**.
    1. В списке **device_id=** выберите идентификатор устройства, по которому вы хотите создать алерт. 
1. В разделе **Настройки алерта** задайте условия срабатывания алерта:
    1. В списке **Условие срабатывания** выберите **Больше**.
    1. В поле **Alarm** введите `70`.
    1. В поле **Warning** введите `50`.
1. По ссылке **Показать дополнительные настройки** раскройте блок дополнительных параметров алерта.
1. В списке **Функция агрегации** выберите **Среднее**.
1. В списке **Окно вычисления** выберите `5 минут`.
1. В блоке **Канал уведомлений** нажмите кнопку **Добавить канал**.
1. В открывшемся окне нажмите кнопку **Создать канал**.
1. В поле **Имя** введите название канала. Например, `my-message-channel`.
1. В списке **Метод** выберите **Email**.

   {% if product == "yandex-cloud" %}Вы также можете настроить уведомления по SMS.{% endif %}
1. В списке **Получатели** выберите учетную запись.    

    Вы можете выбрать несколько получателей уведомлений. В качестве получателей вы можете указать аккаунты пользователей, у которых есть доступ к вашему облаку. Подробнее о том, [как добавить пользователя в {{ yandex-cloud }}](../../iam/operations/users/create.md).
1. Нажмите кнопку **Создать**.  
1. (опционально) Выберите канал уведомлений в таблице и настройте дополнительные параметры уведомлений:    
    * Чтобы включить или отключить отправку уведомлений по определенному статусу алертов, нажмите на соответствующее значение графы **Уведомлять о статусах**:
        * `Alarm`.
        * `Warning`.
        * `OK`.
        * `No data`.
    * Чтобы настроить отправку повторного уведомления, в списке **Уведомлять повторно** выберите, когда вы хотите получить повторное уведомление:
        * `Никогда`.
        * Через `5 минут`.
        * Через `10 минут`.
        * Через `30 минут`.
        * Через `1 час`.
    * Чтобы отредактировать канал уведомлений, нажмите на **...** в правой части строки.  
1. Нажмите кнопку **Создать алерт** в нижней части окна.

Вы можете создавать и настраивать алерты на любую метрику в сервисе {{ monitoring-full-name }}.  

В результате выполнения сценария:  
1. Вы сможете отслеживать показания датчиков на графиках.
1. Если показания датчиков достигнут заданных значений, вы получите уведомления.  

## Удалите созданные облачные ресурсы {#cleanup}

Если вам больше не нужны облачные ресурсы, созданные в процессе выполнения сценария:
* [Удалите реестр в сервисе {{ iot-full-name }}](../../iot-core/operations/registry/registry-delete.md).
* [Удалите устройство в сервисе {{ iot-full-name }}](../../iot-core/operations/device/device-delete.md).
* [Удалите функции в сервисе {{ sf-full-name }}](../../functions/operations/function/function-delete.md).
* [Удалите триггеры функций в сервисе {{ sf-full-name }}](../../functions/operations/trigger/trigger-delete.md).
* [Удалите графики в сервисе {{ monitoring-full-name }}](../../monitoring/operations/).
* [Удалите дашборды в сервисе {{ monitoring-full-name }}](../../monitoring/operations/).
* [Удалите алерты и каналы уведомлений в сервисе {{ monitoring-full-name }}](../../monitoring/operations/).