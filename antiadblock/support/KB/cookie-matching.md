## Куки-матчинг


### Про куки-матчинг

Для показа релевантной рекламы пользователю Яндекс использует его уникальный идентификатор - куку `yandexuid`. 
Так как эта кука ставится на домене `.yandex.ru`, а при шифровании все ссылки запроса за рекламой шифруются под домен партнера, то эта кука в них отсутствует и реклама становится нерелевантной.
Для того, чтобы как-то пробросить данные о куке `yandexuid` в запрос за рекламой по шифрованной ссылке делается куки-матчинг:
* пользователю генерируется уникальный айди и записывается в куку на домене партнера (например, `addruid` в нашем скрипте детекта)
* далее пользователь редиректом или с помощью загрузки картинки проводится по ссылке вида `https://an.yandex.ru/mapuid/otzovik/151219871512198715121987?sign=4191633712`,
где `otzovik` - тэг партнера (поле `EXTUID_TAG` в конфиге партнера), а `151219871512198715121987` - значение куки с айди партнера (куки обозначены в поле `EXTUID_COOKIE_NAMES` в конфиге партнера и в системном конфиге)*[]:

После прохода по этой ручке в [Крипте](https://wiki.yandex-team.ru/Crypta/) появляется маппинг вида: `тэг.внешний_айдишник:yandexuid`. 
При запросе за рекламой через антиадблок-сервис к запросу приклеивается кука с айди посетителя с помощью квери-арга, что понимается крутилкой, которая получает из крипты настоящий `yandexuid`. 

#### Данные по ручке куки-матчинга

Текущие домены для куки-матчинга:

* an.yandex.ru
* statchecker.yandex.ru

**Формирование ссылки для куки-матчинга**

`https://an.yandex.ru/mapuid/<CookieMatchTag>/<ext-uid>?[location=<location>]&jsredir=1&sign=<sign>[&dump-match=1]`

параметр|обязательность|описание
-------- | -------------- | --------
`<CookieMatchTag>`|Да	|Tag для RTB-хоста, создается в Партнерском Интерфейсе (далее — ПИ) менеджером Яндекса. Например, foocom. Обычно уникален для партнера.
`<ext-uid>`|Да|Идентификатор пользователя, присвоенный партнером.
`<location>`|Да, если отсутствует параметр dump-match=1	|URL для перенаправления (редирект). Если передан этот параметр, пользователь перенаправляется на указанный адрес.
`jsredir=1`|Да|Служебный параметр.
`<sign>`|Да|Подпись, гарантирующая достоверность запроса. (на текущий момент не проверяется и может быть любой)
`dump-match=1`|Нет|Включение режима отладки: при наличии ошибки, от an.yandex.ru приходит текстовое сообщение, которое можно увидеть в режиме просмотра HTML-страницы; при отсутствии ошибки — приходит пустой ответ. **Внимание!** В режиме отладки необходимо отключить параметр `<location>`.

#### Участники процесса куки-матчинга со стороны Яндекса

Участник | Компонента | Ответственные
-------- | -------------- | --------
Баннерная крутилка|ручка куки-матчинга, передача запроса на куки-матчинг в Крипту|Антон Полднев, Сергей Анплеев
Крипта|база с куки-матчингом|[Стафф](https://staff.yandex-team.ru/departments/yandex_monetize_big_ic_crypta_yt), Дмитрий Леванов
Анти-фрод|Отвечают за фрод запросов при неправильном куки-матчинге (событие BadCookie и тп)|[Дежурные](https://wiki.yandex-team.ru/JandeksPoisk/Antirobots/bscoll-alert-duty/)

### Проверка куки-матчинга

#### График количества новых пар `ext-uid:yandexuid` perpartner

https://graf.yandex-team.ru/dashboard/db/crypta-identification-views?refresh=1h&orgId=1&from=now-30d&to=now

Если не растет или сначала рос, а потом начал падать, значит с кукиматчингом есть проблемы
Возможные причины:
* идентификаторы у партнера со всякими странными символами. Сейчас пока лучше просить делать идентификаторы только из букв и цифр
* ссылка редиректа на кукиматчинг сама по себе кривая (подпись пока нигде не проверяется, поэтому правильность ее формирования можно не проверять)
* перекукиматчинг кривой: куки либо не протухают через 7 дней, либо не перевыставляются, либо не случается повторный редирект

#### Проверка фрода

Делается в интерфейсе [artmon](https://artmon.bsadm.yandex-team.ru/cgi-bin/artmon_basic.cgi?module=FilterFraudStat2&period_start=2017-07-30&period_start_hour=undefined&period_end=2017-07-31&period_end_hour=undefined&compare_enabled=0&compare_start=2016-06-15&compare_start_hour=undefined&compare_end=2016-06-15&compare_end_hour=undefined&timegroup=hour&periodicity=undefined&query=%7B%22filters%22%3A%7B%22select_table%22%3A%22all%22%2C%22group_by%22%3A%220%22%2C%22fraud_bits%22%3A%5B%220%22%2C%22-1%22%2C%22-2%22%2C%22-3%22%2C%22-4%22%2C%22-5%22%2C%22-6%22%2C%22-7%22%2C%22-8%22%2C%22-9%22%2C%22-10%22%2C%22-11%22%2C%22-12%22%2C%22-13%22%2C%22-14%22%2C%22-15%22%2C%22-16%22%2C%22-17%22%2C%22-18%22%2C%22-19%22%2C%22-20%22%2C%22-21%22%2C%22-22%22%2C%22-23%22%2C%22-24%22%2C%22-25%22%2C%22-26%22%2C%22-27%22%2C%22-28%22%2C%22-29%22%2C%22-30%22%2C%22-31%22%2C%22-32%22%2C%22-33%22%2C%22-34%22%2C%22-35%22%2C%22-36%22%2C%22-37%22%2C%22-38%22%2C%22-39%22%2C%22-40%22%2C%22-41%22%2C%22-42%22%2C%22-43%22%2C%22-44%22%2C%22-45%22%2C%22-46%22%2C%22-47%22%2C%22-48%22%2C%22-49%22%2C%22-50%22%2C%22-51%22%2C%22-52%22%2C%22-53%22%2C%22-54%22%2C%22-55%22%2C%22-56%22%2C%22-57%22%2C%22-58%22%2C%22-59%22%2C%22-60%22%2C%22-61%22%2C%22-62%22%2C%22-64%22%2C%22-65%22%2C%22-66%22%2C%22-67%22%2C%22-68%22%2C%22-69%22%2C%22-70%22%2C%22-71%22%2C%22-72%22%2C%22-73%22%2C%22-74%22%2C%22-75%22%2C%22-76%22%2C%22-77%22%2C%22-78%22%2C%22-79%22%2C%22-80%22%2C%22-81%22%2C%22-82%22%2C%22-83%22%2C%22-84%22%2C%22-85%22%2C%22-86%22%2C%22-87%22%2C%22-88%22%2C%22-89%22%2C%22-90%22%2C%22-91%22%2C%22-92%22%2C%22-93%22%2C%22-94%22%2C%22-95%22%2C%22-96%22%2C%22-97%22%2C%22-98%22%2C%22-99%22%2C%22-100%22%2C%22-101%22%2C%22-102%22%2C%22-103%22%2C%22-104%22%2C%22-105%22%2C%22-106%22%2C%22-107%22%2C%22-108%22%2C%22-109%22%5D%2C%22pageid_filter%22%3A%22149693%22%2C%22source%22%3A%22all%22%2C%22target_type%22%3A%5B%223%22%5D%2C%22device_type%22%3Anull%2C%22page_no%22%3A%5B%22all%22%5D%2C%22mobile%22%3A%220%22%2C%22type_id%22%3A%22all%22%2C%22is_direct%22%3A%22all%22%2C%22is_performance%22%3A%22all%22%2C%22relative%22%3A1%2C%22rel_type%22%3A%220%22%2C%22all_series_in_one_pic%22%3A1%2C%22is_mobile%22%3A%22all%22%7D%7D)

![](https://jing.yandex-team.ru/files/ddemidov/fraud.png)

Смотреть нужно, в первую очередь, на 33 событие фрода - количество IP на одного пользователя, либо `BadCookie` - неправильный формат куки, пробелма с куки-матчингом. Они не должен превышать 5%.
Событие `Af_CheckFrames [Af_CheckClicks] (not uniqually)` можно игнорировать - оно всегда фонит, это какой-то служебный параметр.

### Дебаг проблем с куки-матчингом

Первым делом надо проверить, что на сайте партнера происходит кукиматчинг:

* Удаляем все куки, заходим на сайт.
* В нетворк консоли смотрим, что есть запрос ссылки с `mapuid` внутри в нужном формате (обозначен выше)

Далее можно проверить корректность установки кук:

* Посмотреть expire для кук - рекомендуемый TTL - 7 дней, столько крипта хранит связку `extuid:yandexuid`
* Посмотреть, что происходит при удалении/протухании кук - удалять по одной или комбинациями и смотреть, что все устанавливается так, как ожидается.

#### Как проверить, что куки-матчинг успешен

Проверить корректность кукиматчинга можно дернув ручку в крипте:

```sh
curl -G 'http://idserv.rtcrypta.yandex.net:8080/json/identify' -d 'header.type=IDENTIFY' -d 'header.version=VERSION_4_10' -d 'body.ext_id=gismeteoru.6efaa1cabb34'
```

```
Внимание! Требуется роль: Крипта - Портал - Профили
```

В случае успешного куки-матчинга должен прийти такой ответ:

```json
{
	"header":{
		"type":"IDENTIFY",
		"version":"VERSION_4_10",
		"status":"OK"
	},
	"body":{
		"crypta_id":"3830088755457938900",
		"yuid":"162105786018991",
		"ext_id_attrs":{
			"match_ts":1519807955,
			"synthetic_match":false
		},
		"ext_id_refs":[]
	}
}
```

В ответе надо проверить, что `yuid` совпадает с вашим `yandexuid` (посмотреть можно в своих куках на домене yandex.ru).
Запрос надо запустить несколько раз и убедиться, что всегда отдается один и тот же ответ, так как бывают ситуации, когда за одним `ext-id` закреплено несколько разных `yandexuid` - коллизии при неудачной генерации куки, когда она выдается двум разным людям.

Если `yandexuid` не совпадает с вашим, то надо обратить внимание на поле `"synthetic_match":false` - если там `true`, то отработал серверный куки-матчинг, 
то есть наш запрос за куки-матчингом по ручке mapuid либо не отработал вовсе, либо отработал с ошибкой, и для партенра при запросе рекламы отработал механизм серверного куки-матчинга.
Серверный куки-матчинг - это когда пришел запрос на розыгрыш с `ext-uid`, но его нет в крипте. Тогда крипта генерирует новый чистый `yandexuid` и делает матчинг с ним. Реклама будет нерелевантна, но хотя бы покажется.
Этот механиз включен толькона ряде партнеров (список надо уточнить у Павла Мельничука)

Если куки-матчинг не прошел вовсе, то ответ по ручке будет вот такой:

```(json)
{
	"header":{
		"type":"IDENTIFY",
		"version":"VERSION_4_10",
		"status":"BAD_REQUEST"
	}
}
```


#### Даигностическая информация для передачи в крипту

БК предоставляет возможность сбора диагностики при дергании ручки куки-матчинга. В запросе надо передать дополнительные хедеры `-H 'x-yabs-debug-token: <DEBUG_TOKEN>' -H 'x-yabs-debug-output: proto-text'`.
`<DEBUG_TOKEN>` можно получить по ссылке [https://a-shot2.n.yandex-team.ru/update_debug_cookie](https://a-shot2.n.yandex-team.ru/update_debug_cookie).

Например:

```json
curl 'https://an.yandex.ru/mapuid/otzovik/15128795865847154?sign=4191633712&dump-match=1' -H 'Pragma: no-cache' -H 'Accept-Encoding: gzip, deflate, br' -H 'Accept-Language: ru,en;q=0.9,de;q=0.8,ca;q=0.7' -H 'Upgrade-Insecure-Requests: 1' -H 'User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/62.0.3202.94 YaBrowser/17.11.1.1060 Yowser/2.5 Safari/537.36' -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8' -H 'Cache-Control: no-cache' -H 'Cookie: yandexuid=1621057831516018991; yabs-exp-sid=f26b13cd12345678' -H 'Connection: keep-alive' --compressed -H 'x-yabs-debug-token: f26b13cd12345678' -H 'x-yabs-debug-output: proto-text'


exts {
  tag: "rtcrypta"
  ind: 0
  time: 911
  delta: 924
  success: true
  headers: ""
  response {
    http {
      code: 200
      headers {
        key: "content-type"
        value: "application/json"
      }
      headers {
        key: "Date"
        value: "Wed, 28 Feb 2018 13:19:25 GMT"
      }
      headers {
        key: "transfer-encoding"
        value: "chunked"
      }
      headers {
        key: "Connection"
        value: "keep-alive"
      }
      headers {
        key: "Server"
        value: "h2o/1.1.1"
      }
      entity: "{\n\t\"header\":{\n\t\t\"type\":\"UPLOAD\",\n\t\t\"version\":\"VERSION_4_10\",\n\t\t\"status\":\"OK\"\n\t}\n}"
    }
  }
  request {
    http {
      method: "GET"
      query: "/json/upload?header.type=UPLOAD&header.version=VERSION_4_10&body.type=EXT_ID&body.ttl=86400&body.ext_id.ext_id=otzovik.1500128795865847154&body.ext_id.yuid=1621057831516018991&body.ext_id.track_back_reference=false&body.ext_id.synthetic_match=false"
    }
  }
  timeout: 20000
}
request_id: "\000\005fE\227\226\207S\000\000\027\231&\233\302_"
udp_data {
}
```

Важное в ответе:

```json
{
	"header":{
		"type":"UPLOAD",
		"version":"VERSION_4_10",
		"status":"OK"
	}
}
```

То есть кука была успешно загружена в крипту. Если при этом в крпте ничего нет, то надо отдать им эту простыню и попросить посмотреть в логах на своей стороне.


#### В случае если кукиматчинг работает некорректно
Может вырасти уровень фрода. В [отчете](stat.yandex-team.ru/AntiAdblock/partners_money) это иногда заметно по полю "Разблокированные деньги (фрод)".
Если известен список pageid партнера*, то можно сделать запрос для выяснения вида фрода:
````sql
use hahn;
$pageids = ('102843',);
$script = @@
def get_fraud_bits(n):
    bn = bin(int(n))[2:]
    bits = []
    for i in range(len(bn)):
        if bn[i] == '0':
            continue
        bits.append(len(bn) - i - 1)
    return ', '.join(map(str,bits))
@@;

$callable = Python::get_fraud_bits(
    "(String?)->String",
    $script
);

select 
    Date,
    $callable(dspfraudbits) as bits,
    count(*) as amount
from RANGE([home/logfeller/logs/bs-dsp-log/stream/5min], [2018-06-18T16:00:00], [2018-06-18T17:00:00])
where
    pageid in $pageids and
    CAST(testtag as UInt64) & CAST(Math::Pow(2,49) as UInt64) != 0 and
    (CAST(testtag as UInt64) & 15ul) != 0 and
    countertype='1' and
    win='1' and
    dspid not in ('5', '10')
group by 
    YQL::Concat(YQL::Substring(iso_eventtime, 0, 14), '00:00') as Date, dspfraudbits;
````
Описание [фродбитов есть в вики](https://wiki.yandex-team.ru/users/evdokimenko/rtbantifraud/). Всегда можно попросить помощи у [дежурного Антифрода](https://wiki.yandex-team.ru/JandeksPoisk/Antirobots/bscoll-alert-duty/).

Также наблюдался кейс, когда уровень фрода не увеличивается, но денег от показа рекламы мы не видим - когда в Крипте не находится пары для внешнего ID партнера в [отчете](stat.yandex-team.ru/AntiAdblock/partners_money) это будет заметно по полю "Непопавшие во фрод нулевые uniqid под Адблоком".

*Список pageid партнера:
```sql
SELECT LIST(cast(PageID as String)) as pageids from (select PageID from hahn.[home/yabs/dict/Page] where Name REGEXP '(?:www\.)?otzovik\.com')
```