### FAQ

#### Что такое stat_id?

Если кратко - это механизм разметки трафика (денег).

Сейчас нужно будет таким образом разметить заадблочный разблокированный трафик, чтобы посчитать деньги, которые вам добавило наше решение.

[Документация](https://yandex.ru/support/partner2/web/products-rtb/partner-code.html#partner-code__statId)  
[Как разметить stat-id?](https://tech.yandex.ru/antiadblock/doc/concepts/stat-id-docpage/)  
[Как разметить stat-id для Адфокса?](https://sites.help.adfox.ru/page/45/#statId)  

#### Какие классы можно зашифровать?
Главный критерий, что верстка не поедет, обычно - эти классы НЕ формируются динамически где-то.
То есть везде (в JS, CSS и верстке) используются по полному названию, а не собираются по буквам например )

#### Зачем шифровать статику?
Если мы закриптуем только рекламу, то такие ссылки будет легко вычислить и заблокировать. Для этого шифруется еще часть критичного контента (картинки, css, js), без которых просмотр сайта невозможен. Тогда нельзя будет заблокировать криптованные ссылки банально по длине или какому-то визуальному признаку.

#### При запросе за рекламой получаем 403 от an.yandex.ru
При обработке запроса за рекламой есть несколько проверок того откуда пришёл запрос.
Самый частый кейс - несоответствие `pageId` в запросе домену, с которого отправлен запрос и который указан в параметрах `target-ref` и `page-ref`.
Пример начала запроса за рекламой: `https://an.yandex.ru/meta/93511?target-ref=https%3A%2F%2Fnews.yandex.ru...`
Здесь `pageId` равен `93511`. Обычно такую ошибку легко обнаружить, выполнив запрос в отдельной вкладке - в ответ будет получено `Bad partner domain`.

#### Из каких сетей ожидать запросов от нас?
**NAT64**
Pylon и cryprox ходят на ipv4 адреса (на данный момент это все внешние площадки) из одних и тех же сетей NAT64 - [список](https://racktables.yandex-team.ru/index.php?page=flatip&tab=default&attr_id=10001&attr_value=71170&clear-cf=)</br>
```Firewall macro _NAT64COMMONNETS_```

**Адреса с которых мы ходим как сотрудники из офиса (не VPN!)**

```
95.108.172.0/22
2a02:6b8:0:408::/64
```

#### Как уменьшить нагрузку на сайт от прокси?

Есть несколько способов уменьшить нагрузку на сайт партнера и прокси Антиадблока:

* Отдавать партнерские картинки с помощью `x-accel-redirect` (настройка описана ниже)
* Шифровать не все ссылки на картинки, а настроить в админке процент картинок, которые нужно шифровать (настраивается через поле `Процент шифруемых ссылок на картинки` в админке)
* Изменить период смены ключа шифрования - сделать реже, тогда будет меньше запросов за статикой за счет кеширования 
ресурсов на стороне браузеров пользователей (настраивается через поле `Период смены ключа шифрования` в админке)

##### Про ACCEL_REDIRECT

(Это внутренний механизм nginx, [подробнее](https://www.nginx.com/resources/wiki/start/topics/examples/x-accel/))

На нашей стороне работает так:

1. Получаем запрос за зашифрованной ссылкой
2. Расшифровываем ее
3. Понимаем, что там картинка партнера, попадающая под регулярку из поля "Быстрый редирект" в админке
4. Отдаем в ответ не картинку, а 200-ый ответ с заголовком `x-accel-redirect`, в котором лежит вот такой урл: `/aab-accel-redirect/http://www.mk.ru/media/img/mk.ru/mk_more.gif`

На стороне nginx партнера делается секция:

```nginx
#for Accel-Redirect
    location ~ /aab-accel-redirect/(.*) {
        internal;
        proxy_pass                  $1$is_args$args;
        proxy_cache                 off;
        proxy_ignore_client_abort   off;
        proxy_pass_request_body     on;
        proxy_intercept_errors on;
        error_page 301 302 307 = @handle_redirects;
    }

    location @handle_redirects {
        set $saved_redirect_location '$upstream_http_location';
        proxy_pass $saved_redirect_location;
    }
```

#### Если много запросов OPTIONS от нашего js кода и ответов 405
Если у подключаемого сайта есть поддомены и/или используется `CRYPTED_HOST` который ведет на CDN партнера, то бывает что 
запросы POST из нашего js идут на другой домен, тогда js первым делом проверяет что ему можно слать такой запрос, 
делает он это через OPTIONS, если в ответе приходят необходимые заголовки, то следующий запрос (POST) успешно проходит, 
если нет - не выполняется вовсе. Этими запросами шлется вся статистика у нас по баннерам, количество попыток отрисовки и 
количество успешных отрисовок баннеров, если она к нам не приходит, нам труднее следить за состоянием дел на партнере.

В корневой location nginx добавить ответ на запрос OPTIONS:
```nginx
    if ($request_method = 'OPTIONS') {
        add_header 'Access-Control-Allow-Origin' $DOMAIN always;
        add_header 'Access-Control-Allow-Credentials' 'true' always;
        add_header 'Access-Control-Allow-Methods' 'GET, POST, PUT, OPTIONS' always;
        add_header 'Access-Control-Allow-Headers' 'Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,X-AAB-HTTP-Check' always;
        add_header 'Access-Control-Max-Age' 1728000 always;
        add_header 'Content-Type' 'text/plain charset=UTF-8' always;
        add_header 'Content-Length' 0 always;
        return 200;
    }

```
где `$DOMAIN` - домен под который шифруем, если есть `CRYPTED_HOST` у партнера, то это он.<br>
Официальная документация по кросс-доменным запросам - https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS

#### CRYPTED_HOST

Мы можем подменять хост в ссылках при шифровании.

Было: www.example.com/static/mybest.css
Стало: www.cdn.ex.com/some/crypted/link

Поле в админке называется: `Замена хоста в шифрованной ссылке`

**Важно!** Если на странице партнера есть XHR запросы, то использование фичи в текущем виде невозможно, так как в таком случае
возникают кросс-доменные запросы, которые блокируются по CORS: запрос уходит с сайта partner1.com на сайт partner2.com, под который
мы зашифровали ссылки. В таком случае в XHR нужно ставить `withСredentials`, а партнер со своей стороны должен вернуть заголовок `allow-access-origin`
с соответствующим доменом. По этим причинам фича на текущий момент не используется.

#### Если в наших логах есть 4хх, 5хх коды

Логи с ошибками можно смотреть в нашем эластике:https://ubergrep.yandex-team.ru/app/kibana
Можно использовать фильтры по партнеру и коду ответа: `partner:yandex_news AND response_code:403`

#### Реклама показывается, но в консоли браузера видны блокировки счетчиков

![screenshot](https://wiki.yandex-team.ru/users/ddemidov/FAQ-po-podkljucheniju-i-trablshutingu-partnerov/.files/screenshot2017-07-28at13.12.53.png)

Это внешние счетчики для независимого промера аудитории. ТНС это независимая компания, которая помогает рекламодателям проверять охват их рекламных кампаний, чтобы доказать, что яндекс не врет в своей статистике, когда берет с реклов деньги.

Мы пока не обходим их блокировку.
Они приезжают вместе с показывающим кодом в context.js

#### Как прокси работает с партнерским кодом?
Их библиотеки, которые отрисовывают рекламные блоки, проходят также через нашу прокси, туда подставляются нужные для шифрования на их стороне данные и они, используя тот же алгоритм и ключи шифрования, шифруют ссылки/блоки

Реклама загружается партнерским кодом скриптом context.js - этот скрипт ходит в Директ за рекламой (сам Директ никуда не ходит).
При проходе страницы через прокси скрипт context.js заменяется на context_adb.js, в котором есть вся логика шифрования, которая описана выше

#### Не показывается старый Директ
Для партнеров, которые рисуют директ по старой схеме (плоский директ) в партнерском коде есть завязка на переменную `yandex_ad_format`

Мы же шифруем ```r'\byandex_(?:ad|rtb)': None```  для всех партнеров на бэкенде.

Можно попросить партнера обновить схему подключения к директу хотя бы для заадблочных пользователей.

#### От прокси идут ошибки (5хх) при этом сама прокси доступна и работает
Если партнер видит ошибки на графиках, это не значит, что это мы. 
Партнеры на своих графиках будут видеть ошибки от всех рекламных систем Яндекса, которые есть на сайте. 
Если крутилка начала 500-тить, то это будет видно на графиках. 
Раньше это видели только пользователи, так как напрямую ходили сами в домен an.yandex.ru, например, теперь через шифрованную ссылку через партнера

Определить, какой именно домен в зашифрованных ссылках дает ошибки можно по [партнерскому графику в Solomon](https://solomon.yandex-team.ru/?project=Antiadblock&cluster=cryprox-prod&service=cryprox_actions&dashboard=Antiadblock_partner_dashboard&l.service_id=autoru&autorefresh=y&b=1h&e=):

![](https://jing.yandex-team.ru/files/lawyard/Antiadblock_partner_dashboard__Solomon_2018-11-30_11-59-29.png)

#### Реклама разблокируется, но денег меньше 10% от основных
1. Проверить кукиматчинг. 
Как это сделать написано [здесь](https://wiki.yandex-team.ru/antiadb/proverka-partnerov/)

2. Проверить подтверждение показа. При работе рекламных скриптов вызываются ссылки, которые приводят к появлению в логах определенных записей: 
an.yandex.ru/meta - > логи bs-rtb, bs-dsp с CounterType=0;
an.yandex.ru/rtbcount -> bs-dsp с CounterType=1;
an.yandex.ru/count -> bs-event с CounterType=1,2 (1-показы, 2 -клики).
Мы считаем деньги по тем записям в bs-dsp-log, для которых выполняется условие CounterType=1, поэтому если не вызывается ссылка an.yandex.ru/rtbcount, то деньги от показа рекламы не засчитываются. Проверить это можно если подождать после отрисовки рекламы 2 с (убедившись что она полностью видима), и посмотреть, появляется ли зашифрованный rtbcount-запрос, если нет, то вероятно наше шифрование что-то сломало в рекламных скриптах.

#### Проверка скрипта детекта из консоли браузера

Можно проверять работу скрипта детекта из консоли браузера, скопировав из [официальной документации](https://tech.yandex.ru/antiadblock/doc/concepts/adblock-detect-docpage/) код детекта в консоль и дописав `callback` и `pid` партнера:

```javascript
!function(e,t,n){function o(){var e=Number(new Date),n=new Date(e+36e5*c.time).toUTCString();t.cookie=c.cookie+"=1; expires="+n+"; path=/"}if(n.src){var c={};for(var a in n)n.hasOwnProperty(a)&&(c[a]=n[a]);c.cookie=c.cookie||"bltsr",c.time=c.time||3,c.context=c.context||{},c.callback=c.callback||function(){},function(e){function n(){t.removeEventListener("DOMContentLoaded",n),e()}"complete"===t.readyState||"loading"!==t.readyState&&!t.documentElement.doScroll?e():t.addEventListener&&t.addEventListener("DOMContentLoaded",n)}(function(){var a={blocked:!0,blocker:"UNKNOWN"},r=new(e.XDomainRequest||e.XMLHttpRequest);r.open("get",n.src,!0),r.onload=function(){try{new Function(r.responseText).call(c.context),c.context.init(e,t,c)}catch(e){o(),c.callback(a)}},r.onerror=function(){Number(new Date)-i<2e3&&(o(),c.callback(a))};var i=Number(new Date);r.send()})}}(window,document,{
    src: "https://static-mon.yandex.net/static/main.js?pid=yandex_news",
    cookie: "somecookie",
    callback: aab
});

function aab(result) {
    if (result.blocked === true) {
         console.log('Есть блокировщик')
         console.log(result)
     } else {
         console.log('Нет блокировщика')
         console.log(result)
     }
};
```

Если же нужно проверить, как отработает детект по каким-то новым классам/айдишникам/ссылкам, то можно передать в скрипт детекта json с новыми параметрами (они будут использованы **совместно** с теми, что передаются из прокси), для этого используется опция `detect` следующего формата.

1. Детект по ссылкам: `links: [{type: 'get', src: 'https://an.yandex.ru'}]`. Поле `type` может принимать значения `img` и `script` или любой метод `HTTP`: `get`, `post` итд.
2. Детект по элементам верстки: `custom: ['<div class="adv" />']`. В строке может быть любой `HTML` любой вложенности.
3. Устаревший (deprecated) вариант с указанием атрибутов: `elements: [{class: 'badv', id: 'auto'}]`. Свойства объекта напрямую записываются в специальный невидимый `div`.

При добавлении `debug: true` элемент на котором сработал детект будет выведен в консоль.
После первого выполнения скрипта результат работы кешируется. Для того чтобы проигнорировать кеш используется флаг `force: true`.

Пример:
```javascript
    src: "https://static-mon.yandex.net/static/main.js?pid=yandex_news",
    cookie: "somecookie",
    detect: {
        elements: [{class: "badv"}],
        custom: ['<div class="adfox" />'],
        links: [{type: 'script', src: 'https://an.yandex.ru'}]
    },
    debug: true,
    force: true,
    callback: aab
```

##### Как узнать правило, по которому была определена блокировка?

Скрипт детекта заносит информацию о последнем сработавшем правиле в Local Storage браузера. Запись представляет собой строку, склееную из:
1. Даты и времени (клиентское время)
2. Тип правила: element — блокировка по DOM-элементу, network — по сетевому ресурсу
3. Способ запроса ресурса для типа network: img, script, head, post, get
4. HTML-представление DOM-элемента или URL ресурса

Для скрытия информации от разработчиков блокировщиков, запись перед помещением в Local Storage шифруется с помощью XOR-алгоритма. Ключ шифрования и шифрованные данные склеиваются, используя строку `bHVkY2E=` в качестве разделителя и заносятся в ключ `ludca`.

Чтобы прочесть исходный лог работы скрипта детекта, необходимо:
1. Найти в инспекторе браузера значение по ключу `ludca` в Local Storage или выполнить в консоли `localStorage.getItem('ludca')`
2. Выполнить функцию decode(), передав ей в качестве параметра строку из пункта 1

Пункт 1 нужно выполнить в браузере, где была определена блокировка (партнерский или клиентский браузер). Пункт 2 можно выполнять в любом браузере.

Код функции и пример:
```javascript
var base64alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=",native64alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";function addEquals(a){for(;0!=a.length%4;)a+="=";return a}function decodeBase64(a){a=decodeUInt8String(a);return utf8Decode(a)}
function decodeUInt8String(a){var d=base64alphabet;if(-1!==a.indexOf("+")||-1!==a.indexOf("/"))d=native64alphabet;var b=[],c=0;for(a=a.replace(/[^A-Za-z0-9\-_=+\/]/g,"");c<a.length;){var e=d.indexOf(a.charAt(c++)),f=d.indexOf(a.charAt(c++)),g=d.indexOf(a.charAt(c++)),h=d.indexOf(a.charAt(c++)),k=(f&15)<<4|g>>2,l=(g&3)<<6|h;b.push(String.fromCharCode(e<<2|f>>4));64!==g&&b.push(String.fromCharCode(k));64!==h&&b.push(String.fromCharCode(l))}return b.join("")}
function utf8Decode(a){for(var d=[],b=0;b<a.length;){var c=a.charCodeAt(b);if(128>c)d.push(String.fromCharCode(c)),b++;else if(191<c&&224>c){var e=a.charCodeAt(b+1);d.push(String.fromCharCode((c&31)<<6|e&63));b+=2}else{e=a.charCodeAt(b+1);var f=a.charCodeAt(b+2);d.push(String.fromCharCode((c&15)<<12|(e&63)<<6|f&63));b+=3}}return d.join("")}function xor(a,d){for(var b=[],c=0;c<a.length;c++){var e=a.charCodeAt(c)^d.charCodeAt(c%d.length);b.push(String.fromCharCode(e))}return b.join("")}
function decode(a){var d=decodeUInt8String,b=a.split("dmVyc2lvbg"),c="bHVkY2E=";1!==b.length&&(c="bHVkY2E",d=decodeBase64,a=b[1]);b=a.split(c);a=addEquals(b[1]);b=decodeUInt8String(addEquals(b[0]));return xor(d(a),b)};

decode('6ibfENnx7KL5fC82Um9P77TuW3r2ywTNRTNEInPsDmw6mgUCvagtvX6798v4HrH7RDEu-VlKiKFzPUZaEVQN_A==bHVkY2E=p0mxPPnD24K4CUgWYF9-15TfaEDG8z75cxMDbyfMawBf92BsyYgR2RfN17iMZ92eeRNalilwqIxCDXYqaW8tkI9Aqyr53N2SyQxXDXIHKobThi9A1vwxvT0IZFUaiHoEALoyN83QFp0RzZK5nnLejH4RRpA9Lu3PSB02NWI9eZWFSOUwuJOfzZUJW1NyTiaCxIEpDpelcPZnDXhGGpouD1b7dnGAil3PEd2RooxqxY8wRVqNe2rhxU4fGQg8HSDeylWrabWU0YCdFVxGPg421ZSANBST6TrxalctVE3QIQhT7Ds');
// "Mon, 27 Aug 2018 13:08:46 GMT element <div style="top: -100px; left: -100px; height: 75px; width: 75px; overflow: hidden; position: absolute !important;"><div class="proffitttttttt" id="_R-I-" style="display: none"></div></div>"

decode('2AjvdmVyc2lvbg6ibfENnx7KL5fC82Um9P77TuW3r2ywTNRTNEInPsDmw6mgUCvagtvX6798v4HrH7RDEu-VlKiKFzPUZaEVQN_AbHVkY2Ewr1Dwrs8w7nDgcOfwoLCth9bFmBffsOXwpTDn2lAw4fDsj7DvXITA28nw4xrAF_Dt2Bsw4nCiBHDmRfDjcOXwrjCjGfDncKeeRNawpYpcMKowoxCDXYqaW8twpDCj0DCqyrDucOcw53CksOJDFcNcgcqwobDk8KGL0DDlsO8McK9PQhkVRrCiHoEAMK6MjfDjcOQFsKdEcONwpLCucKecsOewox-EUbCkD0uw63Dj0gdNjViPXnClcKFSMOlMMK4wpPCn8ONwpUJW1NyTibCgsOEwoEpDsKXwqVww7ZnDXhGGsKaLgBbw7hgbsKAworRrdO-0L_TutON07PTjT7SgNK40b7Qg9Glw5t5KcOkw4AATnt4YSZiwprCjE_Cq2TCrcKFwpjDlsKNCA0WOwtyw43Dq8K8djPDm8OpJMK-MUooR07DjmoFScOqaWPDhMKSDcOTEcOVwpLDqcOGIsKewp8tRxDDhXYuw6HDl00');
// "Wed, 03 Oct 2018 12:19:07 GMT element <div style="top: -100px; left: -100px; height: 75px; width: 75px; overflow: hidden; position: absolute !important;"><div label="русские буквы" class="proffitttttttt" id="_R-I-" style="display: none"></div></div>"

decode('azE5N3h4YzA=bHVkY2E=PFRdG1hIURAkUk0XSkhSCUsACA1MTlkCUhF+eixyNn4gf3ZgNnIxVQ1ESlIcWBdfS1RPVhQNAkQOEVgXCwwRWQVWGVYLWClRHVBqVAoRE0RLU1xUGQ0QVUsWTFkLGQVVRlRPVhRfQ1kYEVdYDFgCXktQVVsXDwZUS0JWQgobBhAEVxlEGwoKQB8RUFlYDAtVS1dWWxQXFFkFVhl0FxYXVQVFGWQdGxZCAkVAFygXD1kISBlTEQoGUx9YT1JCWEFDCENQRwxVEEIIER5EHRQFF0tTVVgaQkMXHl9KVh4dTlkFXVBZHV9DWB9FSURCV0xDHlZeUgsMTl0KQUoZARkNVA5JF0UNWAtEH0FKDVdXGlEFVVxPVgoWH0tZTUMIC1kfREJMUB8dEERGXExbDBFNSQpfXVIAVg1VHxFRQwwIEApEHkBWCwwCRAJSF1kdDENYH0VJREJXTFEFH0BWFhwGSEVDTBcQDBdAGAsWGBkcEB4KVV9YAFYRRUtZTUMIC1kfRFBdRE5WAlQNXkEZCg1DWB9FSURCV0xJCl9dUgBWEERLWU1DCAtZH0RTShkBGQ1UDkkXRQ1YC0QfQUoNV1cCQAIcVFYIC01JCl9dUgBWEUVLWU1DCAtZH0QbF1oZCBAeElBXUx0ATV4ORRlfDAwTQ1EeFlobVhpRBVVcT1YKFhADRU1HC0JMHxxUW1YLCk1JCl9dUgBWDVUfEVFDDAgQCkQeSVYLC01JCl9dUgBWEUVLFkpfGUpWBkZSFkQqHSVGGH5vbxc0DV0laQwHLTNVZFloS1FXLCgGPntuAh4KVmlZdAl6RV9DFxhZWAVNTk5UClJvXRlLFWoyRRYHNSJIfwh/DU8RThF3BwJPRAwdN2kSflpAMT5WezNgBBBYXxBYCgMMAVU6NEZSYn50ExE0WAgEXXwhAA5YBwlxYRQqLFgzGggAMzAadwMaAW8QPAdTVhYZEAsQAgJeBxRvFVcoXiEEfV02SAlfH0hVW1cyAkIlYHF6STYAZyRWfWENKyJIRHJAfi8XXhdLFkpfGUpWBkZ3Q10hAShVP3tQQjFXAko4YUFcFAECCDl8d14MLQ9ALlhhRjEABlI5QG9YRV9DFxhZWAVNTk5jGVNRUCkyFkY4WlFgIh4KQCV7aHIaFhF2GgZ2AxE6DggofH97Hk0lQRNoBBBYXxBYCgMMAVUrAmQbdFtDNFMQfxFwegZAKA5bRHdedCEoKB8+S11mEB0VVC1LUVtMQTtbVhYZEAsQAgJeBxRiOQkMe1pCXUAXClBZOQQWWC43WlQhdXEFTx4aHygHUgYyKg1nIGZLVBwhXhdLFkpfGUpWBkZbWmc+FAFRBHN3dVc9AmkRfV9vASo3fS1LSFxMLCtpGVJ1YzVJGVYRBlhERV9DFxhZWAVNTk52PwF4fwAaE1gPQHdfAk1SQRkaeg4WGyZqAGVzXzshU14ja1B6KzshdQAFBBBYXxBYCgMMAVVKUl4yeQsHEEshAAdJamAhIQlDPkVAD0A/O1FaVBJiVykKdA9yC01IHAZHVhYZEBYXDVMOHEt1EzESHwVDSG8aEkhKEX0OYU41GXFWDB4XXxYMXghUFEECPQZlUmUIZT5MMGU/WFNGSQgNVDoMBBBYEBdEG0IDGFcBAkMfUE0ZFh0XEANFTUcLQkwfElBKQxkMTUIeExc9');
// Wed, 02 Oct 2019 11:46:29 GMT
// UNKNOWN
// Refused to evaluate a string as JavaScript because 'unsafe-eval' is not an allowed source of script in the following Content Security Policy directive: "script-src 'self' blob: 'unsafe-inline' https://suggest-maps.yandex.ru https://yandex.ru/ https://suggest-multi.yandex.net https://yastatic.net https://an.yandex.ru https://ads.adfox.ru https://ads6.adfox.ru https://yandex.st https://bs.yandex.ru https://api-maps.yandex.ru https://*.maps.yandex.net https://mc.yandex.ru https://webasr.yandex.net https://pass.yandex.ru 'sha256-c/sReFvsOVXoLnmNX50UK6T2Yrf/TK6UJW5fr5Y2E0M=' 'sha256-dacVja3vZYt/0MZ+OcN4xi6rGl3vsteTYyOcwIF5KXQ=' 'sha256-BWv9SGCkiWhc5dKYxmhl8HVlROhX+17KHyGh+8XhDdc=' 'sha256-Xm/KnJ5DjN0jotyll/JarNQHM1NcWOgDVuSAx/CyIWo=' 'sha256-FzjYyKeTJiuI/azSPxklya8RMNitUlpEiXqIxebRqVo=' 'sha256-SrbhgQJuvSkhWZfipNJQEbnrFq7O4iBm8CMFLf5FqxY=' 'sha256-SaTpEbtL+sOzAC18Pmk/FgCYPK/UzdQhevdFzhl49Xk=' 'sha256-UAqoK1sdwor3iR5/oVO9dJDH27fy/C6k1JRnWKWrcdY=' 'sha256-jcPFlbaoBNB/EaYzLfXyRTMFzqk4THYrcLTM1zfz7as=' 'sha256-FT0AHxbphdqNhz51qr+C9ncEZkTJhCY0nHZiMSCBEk4=' 'sha256-21nYH20h3B0lxSWYYjsUty88GXa1e+U/QiDdC2z0dew=' 'nonce-rBkIq/nrqXbj+zzL7V6MzA==' 'nonce-vzEeU9T1RF4SUTijq1pndQ==' https://yastat.net https://yastat.ru".
```

#### Вышло правило на размер элемента
Например: `div[style="width:1000px;height:120px;”]`
На стороне cryprox мы это починить не можем, но рекомендуем партнеру перенести стили в какой-либо класс, который мы пошифруем.

#### Откуда на морде 95 события
Вообще 95 событие - это win_notice пиксель для РТБшных розыгрышей.
Морда использует его для отправки перед показом, когда загрузился код
То есть последовательность розыгрыша в событиях розыгрыш(15) -> загрузился код(95) -> показ(0) -> клик(1)

#### Где посмотреть описания событий авапса?
В одном месте можно посмотреть [в коде тестов авапса](https://github.yandex-team.ru/QAMS/awaps-ft/blob/master/awaps/libs/HttpUtils.py#L18)  
Фродовые описаны на [вики](https://wiki.yandex-team.ru/Awaps/antifraud/)


#### Как посмотреть актуальные правила на партнера?
Нужно сделать запрос в табличку в YT используя YQL
```sql
select short_rule, action, type, value, added, domains, list_url, options, raw_rule from hahn.[home/antiadb/sonar/sonar_rules] 
where partner = "<service_id>" UNION ALL

select short_rule, action, type, value, added, list_url, options, raw_rule from hahn.[home/antiadb/sonar/general_rules] 
where raw_rule LIKE "%<домен партнера>%"
```
[Сохраненный запрос](https://yql.yandex-team.ru/Queries/5a8bfa3e22bb628fcfdc4bf4)

Запрос правил из определенных листов:
```sql
$partner = "<service_id>";
$domain = "<домен партнера>";
$lists = (
    "https://easylist-downloads.adblockplus.org/ruadlist+easylist.txt",
    "https://filters.adtidy.org/extension/chromium/filters/1.txt",   -- Adguard Russian filter
    "https://raw.githubusercontent.com/AdguardTeam/AdguardFilters/master/RussianFilter/sections/antiadblock.txt"
);
select added, raw_rule, list_url
from hahn.[home/antiadb/sonar/sonar_rules] 
where partner = $partner and list_url in $lists
UNION ALL
select added, raw_rule, list_url
from hahn.[home/antiadb/sonar/general_rules] 
where raw_rule LIKE YQL::Concat(YQL::Concat('%', $domain), '%') and list_url in $lists
```
[Сохраненный запрос](https://yql.yandex-team.ru/Queries/5acf7113ddee599eb2fe4295)

* Не забыть поменять плейсхолдеры
* `service_id` можно посмотреть в админке или конфиге сонара [тут](https://github.yandex-team.ru/AntiADB/adblock-rule-sonar/blob/master/config.py) - это ключи словаря search_regexps

#### Что делать, если резко упали деньги на партнере 
(Переедет в чек-листы)
**Резкое падение денег, как правило, не бывает вызвано выходом нового правила**
1. Посмотреть на [графике партнера](https://solomon.yandex-team.ru/?project=Antiadblock&cluster=cryprox-prod&service=cryprox_actions&dashboard=Antiadblock_partner_dashboard&l.service_id=autoru&autorefresh=y&b=1h&e=) запросы к нам
2. Там же посмотреть, что с ошибками
3. Посмотреть на общие деньги на партнере на [Stat-е](https://stat.yandex-team.ru/AntiAdblock/partners_money)
4. Посмотреть деньги за тот же период прошлого года (возможно сезонное падение денег)
5. Зайти на страницу, посмотреть, есть ли реклама (обновить несколько раз, потому что правила могут скрывать только один тип рекламы)
6. Выяснить, какие были изменения у партнера (спросить несколько раз, даже если говорят, что ничего не катили)
7. Посмотреть, какие есть свежие правила на партнера
8. Если это морда, посмотреть, не поменялось ли количество кампаний, которые мы разблокируем (TODO: Приложить график)

#### Негативный тренд по деньгам на партнере
1. Посмотреть вышедшие с начала тренда правила (Как это сделано - описано [тут](KB/FAQ.md#%D0%9A%D0%B0%D0%BA-%D0%BF%D0%BE%D1%81%D0%BC%D0%BE%D1%82%D1%80%D0%B5%D1%82%D1%8C-%D0%B0%D0%BA%D1%82%D1%83%D0%B0%D0%BB%D1%8C%D0%BD%D1%8B%D0%B5-%D0%BF%D1%80%D0%B0%D0%B2%D0%B8%D0%BB%D0%B0-%D0%BD%D0%B0-%D0%BF%D0%B0%D1%80%D1%82%D0%BD%D0%B5%D1%80%D0%B0))
2. Зайти на страницу, посмотреть, есть ли реклама (обновить несколько раз, потому что правила могут скрывать только один тип рекламы)
3. Спросить у партнера, нет ли каких-то изменений на его стороне или сезонного уменьшения трафика
4. [Проверить корректность кукиматчинга](https://wiki.yandex-team.ru/antiadb/Proverka-partnerov/)

#### Вырос фрод на партнере:
1. Проверить по какому фильтру пошел фрод, для этого выполнить [запрос](https://yql.yandex-team.ru/Operations/XbA7mp3udgGr0UjVRd9ussUfaYhJOh-YEMMr4tiZCfk=) поставив service_id партнера и нужный диапазон дат,
2. Для простоты поиска бита зайти в Charts, выбрать type: pivot, Categories: time, Series: fraudbit, Values: clicks и получим график по которому видно как растет фрод и какой конкретно бит(набор битов)
3. Смотрим интересующие номера битов в [табличке на хане](https://yt.yandex-team.ru/hahn/navigation?path=//home/yabs/dict/GoodEvent&offsetMode=key) или на [wiki](https://wiki.yandex-team.ru/users/gles/bs-filters-mapping/) и осознаем что надо чинить

#### Как закрыть ДЦ?
Описано [тут](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/cryprox/docs/L7_balancers.md#snyatie-anonsov-s-l7-balanserov)
[и тут](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/cryprox/docs/L7_balancers.md#zakrytie-bekendov-cryprox-dlya-l7-balanserov)

#### Выросли тайминги на pylon
Тут может быть две ситуации.</br> 

Сначала нужно посмотреть, актуальна ли проблема на внутренних партнерах.</br>
Если нет, и тайминги выросли только для внешних площадок, скорее всего проблемы с NAT64 схемой. 
В этом случае нужно написать в [NOC](https://t.me/nocduty) и описать проблему - на каких машинах воспроизводится, куда выросли таймауты (бэкенд партнера)

Если проблема наблюдается на всех партнерах, вероятно, дело в туннельной схеме (она не должна работать, но может приехать на машины с какой-нибудь выкладкой - так уже случалось).</br> 
Проверить это можно, посмотрев сетевые интерфейсы на машине командой `ifconfig`. Если среди интерфейсов есть `tun0` - значит включена туннельная схема.</br>
Выключить ее можно, удалив маршрут и потушив интерфейс командой:
```
ip -6 route del fe80::/64 dev tun0
ifconfig tun0 down
```

#### Есть проблемы с показом рекламы AdFox
Можно посмотреть нет ли пустых ответов от AdFox: </br>
Ответы можно найти по хедеру `X-Adfox`</br>
В ответе должен быть [такой JS](https://paste.yandex-team.ru/498074), но может приходить и пустой ответ, который выглядит так:</br>
```
if (window.loadAdFoxBundle) window.loadAdFoxBundle({"data": []});
if (2814512993) { setTimeout(function(){document.close();},100); }
```
Тут может быть 2 ситуации:
1. РСЯ не выкупил показ. Это не баг, просто рекламы нет
2. Неверно настроено рекламное место.</br>
За Адблоком в AdFox может показываться или РСЯ напрямую, или собственные баннеры рекламодателя, заведенные через специальный шаблон "Картинка" ([см. оф.доку](https://tech.yandex.ru/antiadblock/doc/concepts/best-practicies-docpage/#other))

#### Как проверить новый конфиг на продовой верстке партнера 
1. Выкатить новый конфиг на тестинг
2. Зайти на страницу площадки партнера, на которой подключено проксирование и выставить куку `aabtesting=1`
3. Обновить страницу - `cryprox` перенаправит запрос с такой кукой в `cryprox-test`

#### Как посмотреть какие эксперименты проводятся на браузере firefox
1. Зайти на скрытую страницу about:studies и просим отправить себе активные обучения
2. Если требуется повторить у себя, то заходим в about:config и выставляем себе параметры в такие же значения как нам прислали в п1

#### Как расшифровать урл если есть его часть (начальная) и прокси выдает ошибку при расшифровке
1. Выполняем в консоле питона все методы из этого [gist](https://gist.github.yandex-team.ru/sotskov/da402b73966edfa3913c3776fca37308)
2. В конце файла заполняем переменные. В большинстве случаев сможем расшифровать хотя бы часть урла/css class чтоб понять к чему он относится

#### Как проверить динамику адблочной аудитории по логам ЯндексБраузера
Выполнить [запрос](https://yql.yandex-team.ru/Operations/X2h4cS--PAdX3u5blgoIWjvLYAVM9-s16_96ssJ7Fxc=)

#### Как получить процент разблока по отдельным pageId (impId)
1. Посчитать статистику по chevent логам. [Пример запроса](https://yql.yandex-team.ru/Operations/X2hDDi--PAdX3rAI-W-5T2whpIp32z8sQKSeIqRCMtw=)
2. Посчитать статистику по dsp логнам. [Пример запроса](https://yql.yandex-team.ru/Operations/X2SO_J3udrYiDUsaY0YpHhMNCzzXhoMfUQBs5wbAUQw=)
3. Посчитать статистики по blockId по bamboozled-событиям. [Пример запроса](https://yql.yandex-team.ru/Operations/X2I_dJdg8hJMWsNj0rfjmtxNP8ejGDgpt6cz9z0ViAw=)

#### Где посмотреть привязку pageId к страницам Турбо? 
В этой [таблице](https://yt.yandex-team.ru/hahn/navigation?path=//home/webmaster/prod/export/turbo/turbo-hosts) в колонке `Advertising`
