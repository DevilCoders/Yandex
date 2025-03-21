# Выполнить макрос

Макрос в {{ tracker-name }} — это запрограммированный набор операций, который можно запустить на странице задачи. Используйте макросы, чтобы автоматизировать рутинные действия. Макросы позволяют в одно нажатие изменять параметры задачи {% if audience == "internal" %}, создавать типовые комментарии и отправлять письма{% else %} и создавать типовые комментарии{% endif %}.

{% note info %}

У каждой очереди в {{ tracker-name }} свой набор макросов. Если вы не нашли подходящий макрос, [создайте его](../manager/create-macroses.md#section_inq_5b1_x2b).

{% endnote %}

Чтобы выполнить макрос:

1. Откройте страницу задачи.

1. Перейдите к полю для ввода комментария.

1. Выберите макрос из раскрывающегося списка **Макросы**.
    Вы можете выбрать несколько макросов одновременно. Если макросы изменяют один и тот же параметр, к параметру будет применен макрос, который выбран последним.

{% if audience == "internal" %}1. Чтобы отправить письмо с помощью макроса, переключитесь на вкладку **Письмо** и [настройте его параметры](../user/comments.md#send-comment).{% endif %}

1. Чтобы выполнить макрос, нажмите кнопку **Отправить**.



