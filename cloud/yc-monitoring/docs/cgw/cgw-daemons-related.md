# cgw-daemons-related

**Что проверяет**
Состояние systemd сервисов. Если загорелась, значит демон не активен, и возможно, systemd его не может поднять. 
Загорается во время рестарта сервисов. Здесь отображаются сервисы, отвечающие за сбор статистики, логирование, авторекавери. Без них cgw может работать. Поэтому авторекавери срабатывает на эту проверку раз в 10 минут в отведённый слот времени.
