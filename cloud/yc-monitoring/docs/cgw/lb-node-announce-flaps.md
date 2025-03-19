# lb-node-announces-flaps

**Значение**: lb-node активно флапает анонсами.
**Воздействие**: либо мигает какой-то из vip-ов здоровьем, либо lb-node свело мозг и она неправильно вычисляет своё состояние. Это негативно сказывается на работе contrail и vRouter - как минимум, могут страдать виртуалки на гипервизорах.
**Что делать**: найти на графике ([prod](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=loadbalancer_node&l.name=client_grpc__gobgpapi_GobgpApi__*Path_success&l.host=lb-node-*&graph=auto&stack=false&transform=differentiate&b=1d&e=)) больную ноду. Если их счётное количество, то:
1. Проверить, что lb-node не рестартовала недавно. На свежую голову нормально видеть всплеск количества анонсов. Если это не так (или lb-node не должна была рестартовать), смотрим дальше:
2. Предупредить /duty overlay (если они не знают, о чём речь, то конкретно кого:kostya-k или кого:sklyaus).
3. Убедиться, что остальные lb-node в зоне здоровы
4. Рестартовать стек на больной ноде `sudo yavpp-restart`. Возможно, проблема не уйдёт и нужно порестартить ВМ целиком: `sudo systemctl stop vpp ; sudo rebooot`.
Если это все lb-node в зоне, то надо смотреть сколько правил шлют lb-ctrl ([prod](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=loadbalancer_node&l.name=lbctrl_messages&l.host=loadbalancer-node-*&graph=auto&stack=false&transform=differentiate&b=2020-07-13T13%3A13%3A24.424Z&e=2020-07-20T13%3A13%3A24.424Z)) и почему.
