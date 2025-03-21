# Общие вопросы

{% if audience == "external" %}

## Как мигрировать из Яндекс 360 в {{ org-full-name }}

С {{release-date }} 2021 года при подключении {{ tracker-full-name }} автоматически создается организация в {{ org-full-name }}. Администратор может подключить к организации [федерацию удостоверений]({{ link-org-add-federation }}), чтобы сотрудники использовали для входа в {{ tracker-name }} свои рабочие аккаунты. Использование {{ tracker-name }} тарифицируется через [биллинг {{ yandex-cloud }}](../billing/index.yaml).

Если {{ tracker-name }} подключен до {{release-date }}, сервис привязан к организации в Яндекс 360. Постепенно мы перенесем все организации из Яндекс 360 в {{ org-full-name }}, участие администратора организации для этого не требуется. Перед миграцией в {{ org-full-name }} мы предупредим администратора заранее.

{% endif %}

## Как использовать {{ tracker-name }} на мобильном устройстве {#mobile}

Мобильное приложение {{ tracker-name }} доступно:

- в [Google Play]({{ mobile-google-link }}) для мобильных устройств на базе Android версии 8.0 и выше;

- в [App Store]({{ mobile-apple-link }}) для мобильных устройств на базе iOS версии 11.0 и выше.

[Подробнее о мобильном приложении.](mobile.md)

{% if audience == "external" %}

## Как добавить коллег в {{ tracker-name }} {#section_yvw_tyk_nbb}

Коллег, у которых есть аккаунты на Яндексе, можно [пригласить в {{ tracker-name }}](add-users.md#invite_user) прямо из интерфейса сервиса. 

Если в вашей компании есть корпоративная система управления пользователями (например, Active Directory), можно [настроить федерацию удостоверений](add-users.md#federation) для авторизации с помощью SSO, чтобы коллеги могли использовать для [входа в {{ tracker-name }}](user/login.md) свои рабочие учетные записи.

Чтобы добавленные пользователи могли создавать и редактировать задачи, для них нужно [настроить полный доступ](access.md#set).

## Как можно выдать или отозвать полный доступ в {{ tracker-name }} {#full-access}

[Управлять доступом пользователей](access.md#set) можно в интерфейсе {{ tracker-name }} в разделе <q>Управление пользователями</q>. 

Обратите внимание, что сумма платежей за использование сервиса зависит от максимального количества пользователей, у которых был полный доступ в {{ tracker-name }} в течение месяца. Поэтому советуем сначала [отключать доступ](disable-tracker.md) для пользователей, которым он больше не нужен, а затем давать доступ [новым пользователям](add-users.md).

## Помогаете ли вы с внедрением или миграцией из другого сервиса? {#migration}

Мы поможем вам выбрать подходящего партнера для внедрения {{ tracker-name }}. Если вы планируете [подключить к {{ tracker-name }}](add-users.md) более 100 пользователей, мы компенсируем вам часть стоимости внедрения.

{% endif %}

## Как удалить задачу {#section_z1d_r1l_nbb}

Удалить задачу нельзя, но вы можете закрыть ее с подходящей резолюцией. Например, повторяющиеся задачи можно закрыть с резолюцией <q>Дубликат</q>, а созданные по ошибке с резолюцией <q>Отменено</q>. Подробнее о задачах читайте в разделе [Как работать с задачей](user/ticket-in-progress.md).

{% if audience == "external" %}

{% note tip %}

Вы можете [удалить очередь](manager/delete-queue.md) и все входящие в нее задачи. [Перенесите](user/move-ticket.md) ненужные задачи в специальную очередь и затем удалите ее.

{% endnote %}

## Как удалить очередь {#section_hk4_r1l_nbb}

Удалить очередь может только пользователь с доступом к редактированию параметров очереди. Обычно это [администратор](role-model.md) или владелец очереди.

Чтобы удалить очередь:

1. Откройте [страницу очереди](user/queue.md).

1. Слева от имени очереди выберите ![](../_assets/tracker/icon-settings.png) → **Администрирование**. 

1. Перейдите на вкладку **Основные параметры**.

1. Нажмите кнопку **Удалить очередь**.

1. Подтвердите удаление.

Подробнее об удалении очереди читайте в разделе [Удалить очередь](manager/delete-queue.md).

{% endif %}

## Может ли у задачи быть несколько исполнителей? {#section_jlb_yyk_nbb}

Одновременно у задачи может быть только один исполнитель. Вы можете организовать работу нескольких человек одним из способов:

- Если над задачей последовательно работает несколько человек, изменяйте исполнителя в процессе выполнения задачи.
- Если над задачей работает несколько человек одновременно, разбейте ее на подзадачи и укажите для каждой своего исполнителя.

Подробнее о задачах читайте в разделе [Как работать с задачей](user/ticket-in-progress.md).

## У меня нет доступа к задаче {#section_xgr_zng_4bb}

Правила доступа к задачам определяются [настройками очереди](manager/queue-access.md).

Если у вас не хватает прав для доступа на страницу очереди и к ее задачам, обратитесь к владельцу очереди {% if audience == "external" %} или [администратору](role-model.md){% endif %}. Имя владельца очереди обычно указано в сообщении об ограничении доступа.

Если вы можете попасть на страницу очереди, но доступ к некоторым задачам очереди для вас закрыт, возможно, в задачах есть [компоненты с ограниченным доступом](manager/queue-access.md#section_tbh_cs5_qbb). В этом случае также обратитесь к владельцу очереди.

## Как узнать владельца очереди {#section_hgc_xyk_nbb}

Владелец указан на вкладке **Описание** на странице очереди. Подробнее читайте в разделе [Список задач очереди](user/queue.md).

## Почему нельзя использовать виджеты с группировкой по полю <q>Задача</q> {#section_del_widget}

Поле **Задача** содержит название задачи — произвольное значение, которое придумывает пользователь. Почти всегда названия уникальны. Если построить таблицу с группировкой строк или столбцов по множеству уникальных значений (по полю **Задача**), она получится громоздкой и неинформативной.

При создании новых виджетов не используйте поле **Задача** для группировки данных в строках или столбцах [сводных таблиц](user/widgets.md#section_esm_vjp_pz), а также в качестве ключевого параметра для построения [статистики по задачам](user/widgets.md#statistika-po-zadacham). В ближайшее время в виджетах будет отключена возможность настроить  агрегацию (группировку) данных по полю **Задача**. Позже будут удалены все виджеты с такой группировкой.

Если у вас есть виджеты, построенные с группировкой по полю **Задача**, и вы хотите их сохранить, можно:
* заменить ваш виджет [новым виджетом со списком задач](#replace_widget);
* [создать дополнительное поле](#create_new_field) в задачах и хранить в нем данные для построения виджетов с группировкой.

### Создайте новый виджет {#replace_widget}

Если вам нужен простой список задач, замените ваш виджет на [виджет со списком задач](user/widgets#section_ll1_zdp_pz). Для этого:

1. Создайте новый [виджет <q>Задачи</q>](user/widgets#section_ll1_zdp_pz).

1. Поле **Фильтр** заполните так же, как в вашем прежнем виджете. 

1. В поле **Столбцы** выберите параметры задачи, которые нужно отобразить в таблице.

1. Заполните остальные поля и сохраните новый виджет.

1. Удалите прежний виджет.

### Создайте дополнительное поле и настройте группировку по нему {#create_new_field}

Если вы пишете в названиях задач ключевые фразы, которые используете для группировки задач в виджетах, рекомендуем создать отдельное поле для этих фраз. Тогда вы сможете настроить группировку по новому полю вместо поля **Задача**.

1. [Создайте новое поле](user/create-param.md) для хранения информации, которую вы записываете в поле **Задача**.

1. При создании задач сразу вводите данные для построения виджета в новое поле.

1. В существующих задачах перенесите данные в новое поле с помощью [автодействия](user/create-autoaction):

   1. Выберите тип **Обновление задач**.

   1. В разделе **Параметры фильтра** нажмите **Добавить условие**, выберите **Задача** и укажите название задачи (ключевую фразу, которая используется для группировки в виджете).

   1. Нажмите **Добавить действие** и выберите **Изменить значения в полях**. Затем выберите созданное ранее поле и скопируйте в него название задачи.

   1. Создайте аналогичные автодействия для всех возможных ключевых фраз.

1. Если вы создаете задачу с помощью внешнего источника (например, [{{ forms-full-name }}](../forms/create-task.md)), при настройке [интеграции](manager/forms-integration.md) сохраняйте данные для построения виджета в новое поле.

1. [Отредактируйте](user/edit-dashboard.md#section_xz4_bk4_mz) старый виджет: укажите в настройках новое поле вместо поля **Задача**.

{% if audience == "external" %}

## У меня остались вопросы. Кому я могу их задать? {#other-questions}

Пользователи {{ tracker-name }} могут задать вопросы на [странице технической поддержки]({{ link-tracker-support }}). Также вы можете обсуждать интересующие вас темы в нашем сообществе в Telegram: [https://t.me/yandextracker](https://t.me/yandextracker).

{% endif %}