# Как создать очередь

Каждая задача входит в одну из очередей задач. Обычно в очередь группируют задачи, объединенные общей тематикой. Это могут быть задачи проекта, отдела или команды.

{% if locale == "ru" %}

@[youtube](BOLconGWsC0)

{% endif %}

## Создать очередь {#section_mvh_5yb_gz}

{% note warning %}

Владельцем очереди является пользователь, который ее создал. Владелец может настроить очередь самостоятельно или выдать другим пользователям [доступ на изменение параметров очереди](queue-access#set-access). 

{% endnote %}

Чтобы создать новую очередь:

1. На верхней панели {{ tracker-name }} выберите **Очереди** → **+ Создать очередь**.

1. Выберите шаблон очереди. В зависимости от типов задач и статусов, шаблоны входят в одну из категорий:

    - **Разработка** — для задач по разработке и тестированию ПО. 
    - **Работа с людьми и документами** — для процессов техподдержки, HR, согласования документов.
    - **Создание товаров и услуг** — для проектов в сфере услуг и задач производства.
    
    Подробнее о шаблонах и их настройках читайте в разделе [{#T}](workflows.md).
    
    Вы также можете [скопировать очередь](#section_rhb_jjb_mfb) либо [создать очередь вручную](#section_cg3_wpq_w1b).

1. Введите название очереди.

1. Придумайте [ключ очереди](#key). Ключ может состоять только из латиницы и не должен содержать пробелов и специальных символов. 
    
    Старайтесь использовать ключи, которые отражают назначение очереди. Например, ключ `HR` для задач отдела кадров или `SUPPORT` для задач службы поддержки.

    {% note warning %}

    Ключ очереди должен быть уникальным. Новой очереди нельзя присвоить ключ, который принадлежал ранее удаленной очереди.

    {% endnote %}

1. Нажмите кнопку **Создать**. Новая очередь будет доступна в меню **Очереди** на верхней панели {{ tracker-name }}.

1. Если требуется, [измените типы и статусы задач](workflow.md) в очереди и настройте ее параметры.


## Ключ очереди {#key}

Ключ очереди — это уникальный код, по которому можно идентифицировать очередь. Например: `HR`, `SUPPORT`.

Каждой задаче, входящей в очередь, присваивается ключ задачи, который состоит из ключа очереди и порядкового номера.

Ключ очереди можно использовать:

- Для поиска задач — указывать ключ очереди в [условиях фильтров](../user/create-filter.md) и в [языке запросов](../user/query-filter.md). 
- Для прямого доступа к очереди по адресу: `({{ link-tracker-example-2 }})`.

## Скопировать очередь {#section_rhb_jjb_mfb}

Если вы хотите использовать настройки существующей очереди, скопируйте ее:

1. Перейдите в режим создания очереди и нажмите **Скопируйте очередь**.

1. Введите название новой очереди.

1. Придумайте [ключ очереди](#key). Ключ может состоять только из латиницы и не должен содержать пробелов и специальных символов. 
    
    Старайтесь использовать ключи, которые отражают назначение очереди. Например, ключ `HR` для задач отдела кадров или `SUPPORT` для задач службы поддержки.

    {% note warning %}

    Ключ очереди должен быть уникальным. Новой очереди нельзя присвоить ключ, который принадлежал ранее удаленной очереди.

    {% endnote %}

1. Введите ключ или название исходной очереди.

1. Нажмите кнопку **Создать**. Новая очередь будет доступна в меню **Очереди** на верхней панели {{ tracker-name }}.

1. Если требуется, [измените типы и статусы задач](workflow.md) в очереди и настройте ее параметры.

## Создать очередь вручную {#section_cg3_wpq_w1b}

Если для ваших задач не подходит ни один из заготовленных шаблонов, вы можете задать все
 настройки новой очереди самостоятельно:

1. Введите название очереди.

1. Придумайте [ключ очереди](#key). Ключ может состоять только из латиницы и не должен содержать пробелов и специальных символов. 
    
    Старайтесь использовать ключи, которые отражают назначение очереди. Например, ключ `HR` для задач отдела кадров или `SUPPORT` для задач службы поддержки.

    {% note warning %}

    Ключ очереди должен быть уникальным. Новой очереди нельзя присвоить ключ, который принадлежал ранее удаленной очереди.

    {% endnote %}

1. Заполните описание очереди. Описание отображается на странице очереди на вкладке **Описание**.

1. Назначьте владельца очереди. Владелец может изменять настройки очереди.

1. Выберите членов [команды очереди](queue-team.md). Для этого начните вводить логин, имя пользователя или название подразделения и выберите подходящий вариант.
    Заполнять список членов команды нужно, если вы хотите [настроить для них особые права доступа](queue-access.md) или быстрее ставить им задачи.

1. Настройте [уведомления об изменениях задач очереди](subscriptions.md).

1. Настройте для очереди [типы задач и их статусы](workflow.md).

1. Настройте [основные параметры очереди](edit-queue-general.md). Например:

    - Тип и приоритет задач по умолчанию.
    - **Обязательные для отображения поля** — параметры, которые будут отображаться на странице задачи вне зависимости от [настроек пользователя](../user/edit-ticket.md#section_jqw_ppn_jz).

    {% if audience == "internal" %}

    - **Отправка писем** — пользователи смогут [отправлять письма](../user/comments.md#send-comment) на любые адреса прямо со страницы задачи. Текст письма прикрепляется к задаче в виде комментария.

    {% endif %}

    - **Отображать вкладку Коммиты** — включить или выключить вкладку **Коммиты** на странице очереди и на страницах задач. На вкладке отображаются все привязанные к задачам коммиты.
1. Настройте [расширенные возможности очереди](queue-advanced.md), например создайте проекты и компоненты или настройте автоматизацию рутинных действий с задачами.
