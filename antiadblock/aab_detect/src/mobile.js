(/**
 *
 * @param {Window} win
 * @param {Document} doc
 * @param {Object} config
 * @param {string} config.src
 * @param {string} [config.cookie]
 * @param {number} [config.time]
 * @param {*} [config.context]
 * @param {function} [config.callback]
*/
function (win, doc, config) {
    config = config || {};
    var REGION_DOMAINS = ['com.tr', 'com.ge', 'com.am', 'co.il', 'msk.ru', 'spb.ru', 'in.ua'];
    var XMLHttpRequest = win.XDomainRequest || win.XMLHttpRequest;
    var ADB_COOKIES = getTemplateCookies();
    var COOKIE_KEY = 'cookie';
    var TIME_KEY = 'time';
    var CALLBACK_KEY = 'callback';
    var LINKS_KEY = 'links';
    var JSTRACER_URL = ['https://stat', 'ic-mon.yan', 'dex.net/adve', 'rt?rnd=2'].join('');
    var DEFAULT_ANSWER = {'blocked': false, 'blocker': 'NOT_BLOCKED'};
    var SLD = getSld();

    config[COOKIE_KEY] = config[COOKIE_KEY] || 'bltsr';
    config[TIME_KEY] = config[TIME_KEY] || 14 * 24; // 2 недели в часах

    /**
     * Вставлять/удалять куки с ивентом beforeunload
     * @param func
     */
    function actionWithCookies(func) {
        // Проверку наличия addEventListener проводим ниже
        // Если функции нет, то считаем браузер устаревшим
        try {
            win.addEventListener('beforeunload', func);
            win.addEventListener('pagehide', func);
        } catch (e) {
            func();
        }
    }

    /**
     * Проверка явлсяется ли параметр функцией
     * @param {Function} func
     */
    function isFunction(func) {
        return typeof func === 'function';
    }

    /**
     * Костыль, чтобы минификатор на стороне партнерских площадок не смог удалить __ADB_COOKIES__
     */
    function getTemplateCookies() {
        return (window.aaaval, '__ADB_COOKIES__');
    }

    /**
     * Проверка заменился ли шаблон проксей
     * @param {Object} ADB_COOKIES
     */
    function isReplacedCookiesTemplate () {
        return typeof ADB_COOKIES !== 'string';
    }

    /**
     * Получение текущего времени timestamp
     */
    function getTime() {
        return Number(new Date());
    }

    /**
     * Обработка положительного ответа в результате детекта
     * @param {object} blocker
     */
    function callback (blocker) {
        var version = __VERSION__;
        config[CALLBACK_KEY] && config[CALLBACK_KEY](blocker, version);
    }

    /**
     * Получить домен уровня level
     * @param {string} host
     * @param {number} level
     */
    function getDomain(host, level) {
        return host
            .split('.')
            .slice(-level)
            .join('.');
    }

    /**
     * Получить sld
     */
    function getSld() {
        var sld = getDomain(win.location.hostname, 2);
        if (REGION_DOMAINS.indexOf(sld) !== -1) {
            sld = getDomain(win.location.hostname, 3);
        }

        return sld;
    }

    /**
     *  Всегда ставим куку на SLD
     * @param {string} cookieName
     * @param {number|string} value
     * @param {string} expires
     */
    function setCookie(cookieName, value, expires) {
        doc.cookie = cookieName + '=' + value + '; expires=' + expires + '; path=/' + '; domain=.' + SLD + '; SameSite=None; secure';
    }

    function getCookie(name) {
        var cookies = (doc.cookie || '').split('; ');

        for(var i = 0; i < cookies.length; i++) {
            var cookieParts = cookies[i].split('=');
            if (cookieParts[0] === name) {
                return cookieParts[1];
            }
        }

        return undefined;
    }

    /**
     *  Удаляем adb куку дня, которую берем из adbCookies и куку переданного в качестве параметра
     */
    function removeCookies() {
        var adbCookie = isReplacedCookiesTemplate() && ADB_COOKIES.cookieName;
        var expires = new Date(0).toUTCString();

        if (adbCookie) {
            setCookie(adbCookie, 1, expires);
        }
        setCookie(config[COOKIE_KEY], 1, expires);
        // некоторые площадки ставят в десктоп версиях, а кто-то ставит на тачах
        // https://st.yandex-team.ru/ANTIADB-2725
        setCookie('bltsr', 1, expires);
    }

    /**
     *  Удаляем adb deprecated куки, которые берем из adbCookies
     */
    function removeDeprecatedCookies() {
        if (isReplacedCookiesTemplate()) {
            var cookies = ADB_COOKIES.deprecatedCookies || [];
            var expires = new Date(0).toUTCString();

            for (var i = 0; i < cookies.length; i++) {
                setCookie(cookies[i], 1, expires);
            }
        }
    }

    /**
     * Кукиматчинг для партнеров на домене отличном от yandex.tld
     */
    function cookieMatching() {
        var domain = getDomain(win.location.hostname, 2);

        // На yandex.tld кукиматчинг не нужен
        if (/yandex\..+/gi.test(domain)) {
            return;
        }

        var url = 'https://http-check-headers.yandex.ru';
        var cookieName = 'crookie';
        var userMatchedName = 'cmtchd';

        // Если уже закукиматчено, то делать ничего не нужно
        if (getCookie(cookieName) && getCookie(userMatchedName)) {
            return;
        }

        xhrRequest(url, function(xhr) {
            if (xhr.status === 200 && xhr.response) {
                var now = getTime();
                var cookieExpires = new Date(now + 3600000 * config[TIME_KEY]).toUTCString();
                setCookie(cookieName, xhr.response, cookieExpires);

                // userMatchedName ставим на меньшее время, чтобы успеть перезапросить crookie
                cookieExpires = new Date(now + 1800000 * config[TIME_KEY]).toUTCString();
                setCookie(userMatchedName, btoa(String(Number(new Date()))), cookieExpires);
            }
        }, function() {}, true);
    }

    /**
     * Обработка ошибок в процессе детекта
     * @param {string} reason
     */
    function errback(reason, info) {
        var now = getTime();
        // 3600000 === 60 минут
        var cookieExpires = new Date(now + 3600000 * config[TIME_KEY]).toUTCString();

        function _setCookie() {
            setCookie(config[COOKIE_KEY], 1, cookieExpires);
        }

        if (__COOKIE_MATCHING__) {
            try {
                cookieMatching();
            } catch (e) {
                // no catch
            }
        }

        actionWithCookies(_setCookie);

        try {
            info = info || '';
            jstracer(reason, info);
        } catch(e) {
            // no handler
        }

        callback({'blocked': true, 'blocker': 'UNKNOWN'});
    }

    /**
     * Вызывает callback после полной загрузки документа (onDOMContentLoaded)
     */
    function ready(callback) {
        function readyListener() {
            doc.removeEventListener('DOMContentLoaded', readyListener);

            // с задержкой, чтобы выполнится позже скрипта партнера
            setTimeout(callback, 50)
        }

        actionWithCookies(removeDeprecatedCookies);

        if (doc.readyState === 'complete' || (doc.readyState !== 'loading' && !doc.documentElement.doScroll)) {
            callback();
        } else {
            doc.addEventListener && doc.addEventListener('DOMContentLoaded', readyListener);
        }
    }

    /**
     * Отправка логов в jstracer
     * @param {string} reason
     * @param {string} additional
     */
    function jstracer(reason, additional) {
        var raw = {
            'data': {
                'version': __VERSION__,
                'element': reason,
                'additional': additional,
                'inframe': win.top !== win.self,
            },
            'labels': {
                'browser': 'Unknown',
                'device': 'Unknown',
                'blocker': 'MOBILE',
                'pid': 'Unknown',
                'element': 'm0',
                'version': '1'
            },
            'tags': {
                'event_detect_MOBILE': 1
            },
            'location': win.location.href,
            'timestamp': getTime(),
            'service': 'aab_detect',
            'eventName': 'detect_MOBILE',
            'eventType': 'event',
            'value': 1,
            'version': '1'
        };
        var data = win.JSON.stringify(raw);

        if(isFunction(win.navigator.sendBeacon)) {
            win.navigator.sendBeacon(JSTRACER_URL, data);
        } else {
            var xhr = new XMLHttpRequest();
            xhr.open('POST', JSTRACER_URL, true);
            xhr.send(data);
        }
    }

    /**
     * Запрос по урлам из массива links с помощью xhr
     * @param {string} url
     * @param {Function} callback
     * @param {Function} errback
     * @param {boolean} withCredentials
     */
    function xhrRequest(url, callback, errback, withCredentials) {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.withCredentials = withCredentials;

        xhr.onload = function () {
            callback(xhr);
        };

        xhr.onerror = function () {
            errback('XHR request error',  'Status: ' + xhr.statusText + '. URL: ' + url);
        };

        xhr.send();
    }

    function test(counter) {
        counter = counter || 0;
        var urls = config[LINKS_KEY] || [];
        var url = urls[counter];

        if (!url) {
            callback(DEFAULT_ANSWER);
            actionWithCookies(removeCookies);
            return;
        }

        var timeStart = getTime();
        xhrRequest(
            url,
            function () {
                test(counter + 1);
            }, function (reason, additional) {
                var timeEnd = getTime();
                if (timeEnd - timeStart < 2000) {
                    errback(reason, additional);
                } else {
                    test(counter + 1);
                }
            });
    }

    ready(test);
})(window, document, {
    callback: function(result){console.log(result);},
    cookie: 'bltsr',
    // Ссылку на детект необходимо сплитить, так как может попасть под регулярку (https://st.yandex-team.ru/ANTIADB-2701)
    // https://an.yandex.ru/system/context.js
    links: [["https://an.yan", "dex.ru/system/cont", "ext.js"].join("")]
});
