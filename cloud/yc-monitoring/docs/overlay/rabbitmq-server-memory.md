[Алерт rabbitmq-server-memory в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Drabbitmq-server-memory)

Превышен лимит RSS для rabbitmq-server.

Возможно не вычитываются события из очереди (например если «ушла» одна из oct svm в зоне) 

Посмотреть состояние очередей:

`sudo rabbitmqctl list_queues`

Если есть очередь из которой не вычитываются сообщения, то можно удалить её по инструкции из [CLOUD-26577](https://st.yandex-team.ru/CLOUD-26577)