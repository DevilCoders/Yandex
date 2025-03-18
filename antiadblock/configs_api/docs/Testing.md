## Testing

### Как посмотреть на фронт со своим бэкендом
1. Наливаем дев-стенд [инструкция](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/configs_api/samogon/README.md#Как-развернуть-собственную-инсталляцию)
1. Заходим в [namespace test.aabadmin.yandex.ru](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/test.aabadmin.yandex.ru/show/)
1. Кликаем Show Backends
![](https://jing.yandex-team.ru/files/dridgerve/2019-08-16_16-05-10.png)
1. Далее выбираем backend test.aabadmin.yandex.ru
![](https://jing.yandex-team.ru/files/dridgerve/2019-08-16_16-10-05.png)
1. Нажимаем Edit (если Service id совпадает с вашим бекендом админки, то редактировать не нужно, сразу переходим к последнему шагу)
![](https://jing.yandex-team.ru/files/dridgerve/2019-08-16_16-10-34.png)
1. Редактируем поле Service id (указываем свой бекенд, например api-aabadmin23), пишем комментарий и нажимаем Save
![](https://jing.yandex-team.ru/files/dridgerve/2019-08-16_16-12-33.png)
1. Ждем, когда новая версия бекенда станет активной
![](https://jing.yandex-team.ru/files/dridgerve/2019-08-16_16-19-17.png)
1. Если возникла ошибка, то в строке новой версией бекенда будет красная кнопка False, можно на нее кликнуть и посмотреть на ошибки, при этом рабочая версия бекенда будет предыдущей.
1. Далее идем в конфиг фронтового deploy-стейджа [antiadb-front-develop](https://deploy.yandex-team.ru/stage/antiadb-front-develop/config)
1. В случае необходимости делаем переменную окружения BACKEND_API_URL равной `https://test.aabadmin.yandex.ru/` и деплоим стейдж с новым конфигом
1. Стенд доступен по [ссылке](https://develop.antiblock.yandex.ru/)
