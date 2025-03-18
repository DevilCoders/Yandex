## L7 балансеры cryprox.yandex.net

### Общая инфорамция

1. [Тикет](https://st.yandex-team.ru/ANTIADB-1925)
1. [Awacs-namespace](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/)
1. [Конфиг Upstream slbping (для чеков L3-балансера)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/upstreams/list/slbping/show/)
1. [Конфиг Upstream default (full mesh)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/upstreams/list/default/show/)
1. [Конфиг балансера (на примере MAN)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/balancers/list/cryprox.yandex.net_man/show/)

### Сервисы для балансеров

1. [rtc_balancer_cryprox_yandex_net_sas](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_cryprox_yandex_net_sas/)
1. [rtc_balancer_cryprox_yandex_net_vla](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_cryprox_yandex_net_vla/)
1. [rtc_balancer_cryprox_yandex_net_man](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_cryprox_yandex_net_man/)
1. [rtc_balancer_cryprox_yandex_net_iva](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_cryprox_yandex_net_iva/)
1. [rtc_balancer_cryprox_yandex_net_myt](https://nanny.yandex-team.ru/ui/#/services/catalog/rtc_balancer_cryprox_yandex_net_myt/)

### Графики балансеров

1. График запросов в L7-балансер
1. График запросов из L7 в бекенды (requests_to_man, requests_to_sas, requests_to_vla, requests_to_iva, requests_to_myt)

Все ссылки [здесь](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/monitoring/common/)

### Снятие анонсов с L7-балансеров

Для того, чтобы L3-балансер отстрелил L7-балансеры с них нужно снять анонс. В текущей настройке L3 шлет /ping и ожидает получить 200.   
Анонсы снимаются для группы L7-балансеров в одной локации [здесь](https://nanny.yandex-team.ru/ui/#/its/locations/balancer/cryprox_yandex_net/common/)   
![](https://jing.yandex-team.ru/files/dridgerve/its_anons_1.png)   


Выбираем локацию и жмем Apply    

![](https://jing.yandex-team.ru/files/dridgerve/its_anons_2.png)  



Для удаления жмем кнопку с корзиной   

![](https://jing.yandex-team.ru/files/dridgerve/its_anons_3.png)     
Такй механизм снятия анонсов работает, если используется модуль **slb_pin_macro** в [Upstream slbping](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/cryprox.yandex.net/upstreams/list/slbping/show/)


### Закрытие бекендов cryprox для L7-балансеров

Локации бекендов закрываются с помощью [ITC-дашборда](https://nanny.yandex-team.ru/ui/#/l7heavy/cryprox.yandex.net/)    
Дефолтные веса настроены в соответствии с количеством инстансев в каждом ДЦ, веса нормируются к 100.    
Для закрытия ДЦ нужно установить соотвествующий вес в 0 и нажать кнопку сохранить, веса открытых ДЦ автомотически перенормируются
Чтобы вернуть веса к дефолту, нужно кликнуть кнопку Reset weights to default и нажать кнопку сохранить.   
Балансеры перечитывают конфиг с весами бекендов раз в секунду.  
Чтобы закрыть конкретный бэкенд надо зайти на него по ssh и выполнить команду `slb close`


### Баннилка запросов

Открываем секцию common [здесь](https://nanny.yandex-team.ru/ui/#/its/locations/balancer/cryprox_yandex_net/common/) 
Пишем правило для бана и нажимаем Apply    
![](https://jing.yandex-team.ru/files/dridgerve/its_ban_1.png)
Для удаления жмем кнопку с корзиной   
Описание правил [здесь](https://wiki.yandex-team.ru/balancer/Cookbook/banrequests/)  
Пример можно посмотреть в документации ПКОДа [здесь](https://wiki.yandex-team.ru/pcode/pcode-traffic-management/#banilkazaprosov)   


### Логирование

Сейчас access и error логи пишутся в папку /usr/local/www/logs/ на каждом инстансе балансера

### Дебаг проблем


TODO: https://st.yandex-team.ru/ANTIADB-1993 сделать дашборд с 5xx инстансов балансера
1. На [графиках балансера](https://nda.ya.ru/3VoP5p) есть ошибки 5xx, а в соломоне нет
1. После определения локации, в которой проблема, заходим в соответствующий сервис балансера и открываем список инстансов
1. Строим [график для каждого инстанса](https://nda.ya.ru/3VoHgg) для определения проблемного
1. Заходим по ssh на проблемный инстанс и смотрим логи
1. В зависимости о  проблемы идем или в чат RTC Support или в чат L7-balancer-support
