# Как отключить платные возможности {{ tracker-name }}

{% note info %}

Чтобы не платить за использование {{ tracker-name }}, вы можете отключить платные возможности и оставить сотрудникам только бесплатный доступ в режиме [<q>Только чтение</q>](access.md#readonly). Полностью отключить {{ tracker-name }} невозможно. 

{% endnote %}

Чтобы прекратить использование платных возможностей {{ tracker-full-name }}, отзовите у сотрудников организации полный доступ в {{ tracker-name }}. Если полный доступ есть не более чем у 5 пользователей, использование {{ tracker-name }} не тарифицируется. Подробнее читайте в разделе [{#T}](pricing.md).

1. На верхней панели {{ tracker-name }} нажмите ![](../_assets/tracker/tracker-burger.png) → **Управление пользователями**.

1. В разделе **Все пользователи** для сотрудников, у которых нужно отозвать доступ в {{ tracker-name }}, в столбце **Доступ** выберите **Только чтение**. 

{% if audience == "draft" %}1. Чтобы отозвать доступ у всех сотрудников сразу, на левой панели выберите раздел **Отделы**, затем в строке **Все сотрудники** выберите **Только чтение**.{% endif %}

После того как у сотрудников будет отозван полный доступ в {{ tracker-name }}, они не смогут создавать и обновлять задачи. Возможность просматривать информацию в {{ tracker-name }} сохранится.