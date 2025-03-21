# Трейл

Трейл — это ресурс {{ at-name }}, который отвечает за сбор и доставку аудитных логов ресурсов {{ yandex-cloud }} в [бакет](../../storage/concepts/bucket.md) {{ objstorage-name }}{% if product == "yandex-cloud" %}, [лог-группу](../../logging/concepts/log-group.md) {{ cloud-logging-name }} или [поток данных](../../data-streams/concepts/glossary.md#stream-concepts) {{ yds-name }}{% endif %}.

## Область сбора аудитных логов {#collecting-area}

В настройках трейла можно выбрать область сбора аудитных логов:
* организация — аудитные логи всех ресурсов во всех облаках организации;
* облако — аудитные логи ресурсов, которые находятся в выбранных каталогах облака;
* каталог — аудитные логи каталога, в котором находится трейл.

Трейл будет собирать и загружать в бакет{% if product == "yandex-cloud" %}, лог-группу или поток данных{% endif %} аудитные логи всех [ресурсов](./events.md), которые находятся в заданной области сбора аудитных логов, в том числе ресурсов, которые будут добавлены в область после создания трейла. 

Для тех ресурсов, которые были добавлены в область сбора аудитных логов уже после создания трейла, сбор аудитных логов запустится трейлом автоматически. 

## Объект назначения {#target}

Каждый трейл загружает аудитные логи только в один объект назначения: бакет{% if product == "yandex-cloud" %}, лог-группу или поток данных{% endif %}.

При загрузке аудитных логов в бакет {{ at-name }} формирует файлы аудитных логов приблизительно раз в 5 минут. Трейл запишет все [события](./events.md), которые произошли за это время с ресурсами облака, в один или несколько файлов. Если никакие события за этот период не произойдут, файлы не сформируются.

{% if product == "yandex-cloud" %}
В лог-группу {{ at-name }} загружает аудитные логи в режиме, близком к реальному времени.

От типа объекта назначения зависит структура и содержимое сообщения, в котором {{ at-name }} передает аудитные логи:
* для бакета — в файле находится массив [JSON-объектов](./format.md#scheme) аудитного лога;
* для лог-группы — в сообщении находится только один JSON-объект аудитного лога.
{% endif %}

Каждый трейл действует независимо от других трейлов. Используя несколько трейлов, можно разграничивать доступ к разным группам логов для пользователей и сервисов в соответствии с требованиями политики ИБ.

## Настройки трейла {#trail-settings}

Трейл содержит в себе все настройки аудитного лога:
* **Имя** — обязательный параметр.
* **Описание** — опциональный параметр.
* Блок **Фильтр**:
    * **Ресурс** — значения `Организация`, `Облако` или `Каталог`.
    * Для значения `Организация`:
        *  **Организация** – имя текущей организации. Значение подставляется автоматически.
    * Для значения `Облако`:
        * **Облако** — имя облака, в котором находится текущий трейл. Значение подставляется автоматически.
        * **Каталоги** — каталоги, для ресурсов в которых трейл будет собирать аудитные логи. Если не указать ни один каталог, то трейл будет собирать аудитные логи всех ресурсов в облаке.
    * Для параметра `Каталог`:
        * **Каталог** — имя каталога, в котором находится трейл. Значение подставляется автоматически.
* Блок **Назначение**:
    * **Назначение** — {% if product == "yandex-cloud" %}значения{% endif %}{% if product == "cloud-il" %}значение{% endif %} `{{ objstorage-name }}`{% if product == "yandex-cloud" %}, `{{ cloud-logging-name }}` или `{{ yds-name }}`{% endif %}.
    * Для значения `{{ objstorage-name }}`:
        * **Бакет** — имя бакета.
        * **Префикс объекта** — необязательный параметр, участвует в [полном имени](./format.md#log-file-name) файла аудитного лога.
{% if product == "yandex-cloud" %}
    * Для значения `{{ cloud-logging-name }}`:
        * **Лог-группа** — имя лог-группы.
    * Для значения `{{ yds-name }}`:
        * **Поток данных** — имя потока данных.
{% endif %}
* Блок **Сервисный аккаунт** — сервисный аккаунт, от имени которого будет выполняться загрузка аудитных логов в бакет {% if product == "yandex-cloud" %}, лог-группу или поток данных{% endif %}. Если аккаунту нужны дополнительные роли, появится предупреждение с перечислением ролей.

## Статус трейла {#status}

У трейла может быть два статуса: `active` и `error`.
Статусы обозначают состояние самого трейла и никак не относятся к событиям ресурсов, аудитные логи которых трейл собирает:
* `active` — трейл работает и собирает аудитные логи с доступных ресурсов;
* `error` — возможно нарушение в работе объектов назначения трейла или самого трейла. 

## Что дальше {#whats-next}

* Узнайте о [формате аудитных логов](./format.md).
* Узнайте о [событиях](./events.md).
