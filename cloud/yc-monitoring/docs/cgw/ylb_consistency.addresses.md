# ylb_consistency.addresses

**Значение:** Означает что пользователь удалил балансер, но мы по каким-то причинам не отпустили address, и оставляем владение за удаленным балансером.
**Воздействие:** Занимаем адрес в пуле, и никак его не используем.
**Что делать:**
1. Грепать логи по этому балансеру/адресу, и разобраться почему адресс не удалился ([prod](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/yandexcloud-prod-log) [preprod](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/yandexcloud-pre-production-log))  ??пример бы выражения с фильтром ещё??
2. Нужно руками отпустить адрес, как это лучше делать - вопрос к /duty vpc
