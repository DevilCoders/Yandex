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
    var localConfig = {};
    for (var key in config) {
        if (config.hasOwnProperty(key)) {
            localConfig[key] = config[key];
        }
    }

    var REGION_DOMAINS = ['com.tr', 'com.ge', 'com.am', 'co.il', 'msk.ru', 'spb.ru', 'in.ua'];
    var XMLHttpRequest = win.XDomainRequest || win.XMLHttpRequest;

    var SRC_KEY = 'src';
    var TRUSTED_SRC_KEY = 'trusted';
    var COOKIE_KEY = 'cookie';
    var TIME_KEY = 'time';
    var PORTION_KEY = 'portion';
    var CONTEXT_KEY = 'context';
    var CALLBACK_KEY = 'callback';
    var CYCADA_COOKIE_KEY = 'cycada';
    var LOCAL_STORAGE_VERSION_KEY = 'beerka';
    var LOCAL_STORAGE_INFO_KEY = 'ludca';
    var LOCAL_STORAGE_DELIMITER = 'bHVkY2E=';
    var JSTRACER_URL = ['https://stat', 'ic-mon.yan', 'dex.net/adve', 'rt?rnd=1'].join('');

    localConfig[COOKIE_KEY] = localConfig[COOKIE_KEY] || 'bltsr';

    try {
        if (({}).toString.call(localConfig[COOKIE_KEY]) !== '[object Array]') {
            localConfig[COOKIE_KEY] = [localConfig[COOKIE_KEY]];
        }
    } catch (e) {
        // nothing
    }

    localConfig[TIME_KEY] = localConfig[TIME_KEY] || 14 * 24; // 2 недели в часах
    localConfig[CONTEXT_KEY] = localConfig[CONTEXT_KEY] || {};

    var oldCallback = localConfig[CALLBACK_KEY];
    localConfig[CALLBACK_KEY] = callback;

    if (!localConfig[SRC_KEY]) {
        errback('Empty config.src');
        return;
    }

    var url = localConfig[SRC_KEY];
    var trustedUrl = localConfig[TRUSTED_SRC_KEY];

    var steps = [];
    var currentTime = getTime();

    /**
     * Сохраняем события в массив, чтобы потом отправить назад для отладки
     * @param {string} description
     */
    function logStep(description) {
        steps.push({d: description, t: getTime() - currentTime});
    }

    /**
     * Проверка явлсяется ли параметр функцией
     * @param {Function} func
     */
    function isFunction(func) {
        return typeof func === 'function';
    }

    /**
     * XOR строки text по ключу seed
     * @param {string} text
     * @param {string} key
     */
    function xor(text, key) {
        var result = [];

        for (var i = 0; i < text.length; i++) {
            var xored = text.charCodeAt(i) ^ key.charCodeAt(i % key.length);

            result.push(win.String.fromCharCode(xored));
        }

        return result.join('');
    }

    /**
     * Получение текущего времени timestamp
     */
    function getTime() {
        return win.Number(new win.Date());
    }

    /**
     * Генерация Id на основе времени. Меняется раз в 40 минут
     */
    function generateUniqueId() {
        var _40min = 1000 * 60 * 40;
        return ((getTime() / _40min).toFixed() * _40min).toString(36).slice(0, 10);
    }

    /**
     * Ждем time времени шажками по 100мс
     * @param {Function} callback
     * @param {number} time
     */
    function timeout(callback, time) {
        var start = getTime();
        var timer = null;
        var canceled = false;

        function cancel() {
            canceled = true;
            win.clearTimeout(timer);
        }

        function wait() {
            if (canceled) {
                return;
            }

            if (getTime() - start >= time) {
                callback();
                return;
            }

            timer = win.setTimeout(function () {
                wait();
            }, 100);
        }

        wait();

        return cancel;
    }

    /**
     * Запись в localStorage
     * @param {string} name
     * @param {string} value
     * @param {boolean} encode
     */
    function storeInLocalStorage(name, value, encode) {
        try {
            if (win.localStorage) {
                if (encode) {
                    var seed = generateUniqueId();
                    value = win.btoa(seed) + LOCAL_STORAGE_DELIMITER + win.btoa(xor(value, seed));
                }
                win.localStorage.setItem(name, value);
            }
        } catch (e) {
            // localStorage is disabled
        }
    }

    /**
     * Обработка положительного ответа в результате детекта
     * @param {object} blocker
     */
    function callback (blocker) {
        var version = "2.1.0";
        oldCallback && oldCallback(blocker, version);
        storeInLocalStorage(LOCAL_STORAGE_VERSION_KEY, version);
    }

    /**
     * Получить куки по имени
     * @param {string} name
     */
    function getCookie(name) {
        var cookies = doc.cookie.split('; ');
        for (var i = 0; i < cookies.length; i++) {
            var pair = cookies[i].split('=');
            if (pair[0] === name) {
                return pair.slice(1).join('=');
            }
        }

        return null;
    }

    /**
     * Получить домен уровня level
     * @param {string} host
     * @param {number} level
     */
    function getDomain(host, level) {
        if (!level) {
            return host;
        }

        return host
            .split('.')
            .slice(-level)
            .join('.');
    }

    /**
     * Обработка ошибок в процессе детекта
     * @param {string} reason
     */
    function errback(reason, info) {
        var now = getTime();
        // 3600000 === 60 минут
        var cookieExpires = new win.Date(now + 3600000 * localConfig[TIME_KEY]).toUTCString();
        var cookiesToSet = [];

        // Всегда ставим куку на SLD
        var sld = getDomain(win.location.hostname, 2);
        if (REGION_DOMAINS.indexOf(sld) !== -1) {
            sld = getDomain(win.location.hostname, 3);
        }

        function setCookie() {
            for (var i = 0; i < cookiesToSet.length; i++) {
                var cookie = cookiesToSet[i];
                doc.cookie = cookie.name + '=' + cookie.value + '; expires=' + cookie.expires + '; path=/; domain=.' + cookie.domain + '; SameSite=None; secure';
            }
        }

        var cookies = localConfig[COOKIE_KEY];
        for (var i = 0; i < cookies.length; i++) {
            cookiesToSet.push({
                name: cookies[i],
                value: 1,
                expires: cookieExpires,
                domain: sld
            });
        }

        var cycada = getCookie(CYCADA_COOKIE_KEY);
        if (cycada) {
            // Делаем куку сессионной
            doc.cookie = CYCADA_COOKIE_KEY + '=' + cycada + '; path=/; domain=.' + sld + '; SameSite=None; secure';

            cookiesToSet.push({
                name: CYCADA_COOKIE_KEY,
                value: cycada,
                expires: new win.Date(0).toUTCString(),
                domain: sld
            });
        }

        // Проверку наличия addEventListener проводим ниже
        // Если функции нет, то считаем браузер устаревшим
        try {
            win.addEventListener('beforeunload', setCookie);
            win.addEventListener('pagehide', setCookie);
        } catch (e) {
            setCookie();
        }

        try {
            info = info || '';
            jstracer(reason, info);
            storeInLocalStorage(LOCAL_STORAGE_INFO_KEY, [new win.Date().toUTCString(), 'UNKNOWN', reason, info, getWindowLocation()].join('\n'), true);
        } catch(e) {
            // no handler
        }

        var result = {'blocked': true, 'blocker': 'UNKNOWN'};
        callback(result);
    }

    /**
     * Получить location для логирования
     */
    function getWindowLocation() {
        if (win.location) {
            if (typeof win.location.toString === 'function') {
                return win.location.toString();
            } else {
                return win.location.href || '';
            }
        } else {
            return '';
        }
    }

    /**
     * Вызывает callback после полной загрузки документа (onDOMContentLoaded)
     */
    function ready(callback) {
        logStep('START');
        function readyListener() {
            logStep('DOM LOADED');
            doc.removeEventListener('DOMContentLoaded', readyListener);
            callback();
        }

        if (doc.readyState === 'complete' || (doc.readyState !== 'loading' && !doc.documentElement.doScroll)) {
            logStep('DOC READY');
            callback();
        } else {
            try {
                doc.addEventListener && doc.addEventListener('DOMContentLoaded', readyListener);
            } catch (e) {
                logStep('Error while subscribing DOMContentLoaded');
                errback(e && e.stack);
            }
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
                'version': "2.1.0",
                'element': reason,
                'additional': additional,
                'inframe': win.top !== win.self,
                'steps': steps
            },
            'labels': {
                'browser': 'Unknown',
                'device': 'Unknown',
                'blocker': 'INLINE',
                'pid': 'Unknown',
                'element': 'i0',
                'version': '1'
            },
            'tags': {
                'event_detect_INLINE': 1
            },
            'location': win.location.href,
            'timestamp': getTime(),
            'service': 'aab_detect',
            'eventName': 'detect_INLINE',
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
     * Запрос static-mon с помощью xhr
     * @param {string} url
     * @param {Function} callback
     * @param {Function} errback
     */
    function xhrRequest(url, callback, errback) {
        var xhr = new XMLHttpRequest();

        // UNSENT === 0
        if (xhr.readyState !== 0) {
            throw new Error('XHR constructor error');
        }

        xhr.open('GET', url, true);

        // OPENED === 1
        // Нельзя переопределять send по тупому
        if (xhr.readyState !== 1 || xhr.send !== XMLHttpRequest.prototype.send) {
            throw new Error('XHR functions were reassigned');
        }

        xhr.withCredentials = true;

        xhr.onload = function () {
            callback(xhr);
        };

        xhr.onerror = function () {
            errback('XHR request error',  'Status: ' + xhr.statusText);
        };

        xhr.send();
    }

    try {
        if(!isFunction(win.addEventListener) ||
           !isFunction(win.getComputedStyle) ||
           !isFunction(win.Function.prototype.bind) ||
           (doc.documentMode && doc.documentMode <= 10)) {
            var result = {'blocked': false, 'blocker': 'NOT_BLOCKED'};
            callback(result);
            return;
        }
    } catch (e) {
        // nothing
    }

    var cancel = null;

    if (localConfig.timeout) {
        // Cамый страшный вариант
        // Если блокировщик ломает что-то, мы падаем в timeout
        cancel = timeout(function () {
            logStep('TIMEOUT');
            errback('Timeout');
        }, 20000);
    }

    if (localConfig[PORTION_KEY]) {
        try {
            let STORAGE_IS_IN_PORTION_KEY = 'adbIsInPortion';
            let STORAGE_PORTION_KEY = 'adbPortion';

            var isUserInPortion = win.localStorage.getItem(STORAGE_IS_IN_PORTION_KEY)
            var lastPortion = Number(win.localStorage.getItem(STORAGE_PORTION_KEY))

            if (isUserInPortion === null || lastPortion !== localConfig[PORTION_KEY]) {
                isUserInPortion = Math.random() < localConfig[PORTION_KEY];
                storeInLocalStorage(STORAGE_IS_IN_PORTION_KEY, isUserInPortion, false);
                storeInLocalStorage(STORAGE_PORTION_KEY, localConfig[PORTION_KEY], false);
            }

            if (isUserInPortion === false || isUserInPortion === 'false') {
                callback({'blocked': false, 'blocker': 'NOT_BLOCKED'});
                return;
            }
        } catch (e) {}
    }

    ready(function () {
        try {
            var timeStart = getTime();

            logStep('XHR REQUEST');

            xhrRequest(
                url,
                function (xhr) {
                    try {
                        cancel && cancel();
                        logStep('XHR ANSWER');

                        // Защищаем скрипт присылая с сервера его content-lenght
                        var expectedLength = win.Number(xhr.getResponseHeader('content-lenght'));
                        var currentLength = xhr.responseText.length;
                        if (!expectedLength || expectedLength < 32000 || expectedLength !== currentLength) {
                            return errback('Header length doesn\'t match', expectedLength + ' - ' + currentLength);
                        }

                        var fn = new win.Function(xhr.responseText);
                        fn.call(localConfig[CONTEXT_KEY]);
                        localConfig[CONTEXT_KEY].init(win, doc, localConfig);
                    } catch (e) {
                        errback(e && e.message);
                    }
                }, function (reason, additional) {
                    try {
                        cancel && cancel();
                        logStep('XHR ERRBACK');
                        var timeEnd = getTime();
                        if (timeEnd - timeStart < 2000) {
                            // Делаем запрос на доверенный url, чтобы проверить доступность интернета
                            if (trustedUrl) {
                                xhrRequest(
                                    trustedUrl,
                                    function () {
                                        logStep('TRUSTED XHR ANSWER');
                                        debugger;
                                        // Интернет доступен, но static-mon получить не удалось -> ставим куку
                                        errback(reason, additional);
                                    }, function () {
                                        debugger;
                                        // Ошибка загрузки -> что-то не так. Просто выходим
                                        var answer = {'blocked': false, 'blocker': 'NOT_BLOCKED'};
                                        callback(answer);
                                    });
                            } else {
                                errback(reason, additional);
                            }
                        } else {
                            var answer = {'blocked': false, 'blocker': 'NOT_BLOCKED'};
                            callback(answer);
                        }
                    } catch (e) {
                        errback(e && e.message);
                    }
                });
        } catch (e) {
            errback(e && e.stack);
        }
    });
})(window, document, {

});
