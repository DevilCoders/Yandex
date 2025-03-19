// ==UserScript==
// @name         TMHits handler
// @namespace    http://tampermonkey.net/
// @homepage     https://wiki.yandex-team.ru/evgenijjgrechnikov/text-machine-hits-viewer/
// @version      0.13
// @description  Provides a link to the viewer of TMHits
// @author       grechnik
// @match        https://yandex.ru/search/*
// @match        https://yandex.by/search/*
// @match        https://yandex.kz/search/*
// @match        https://yandex.ua/search/*
// @match        https://yandex.com.tr/search/*
// @match        https://yandex.com/search/*
// @match        https://yandex.uz/search/*
// @match        https://yandex.eu/search/*
// @match        https://hamster.yandex.ru/search/*
// @match        https://hamster.yandex.by/search/*
// @match        https://hamster.yandex.kz/search/*
// @match        https://hamster.yandex.ua/search/*
// @match        https://hamster.yandex.com.tr/search/*
// @match        https://hamster.yandex.com/search/*
// @match        https://hamster.yandex.uz/search/*
// @match        https://hamster.yandex.eu/search/*
// @match        https://*.hamster.yandex.ru/search/*
// @match        https://*.hamster.yandex.by/search/*
// @match        https://*.hamster.yandex.kz/search/*
// @match        https://*.hamster.yandex.ua/search/*
// @match        https://*.hamster.yandex.com.tr/search/*
// @match        https://*.hamster.yandex.com/search/*
// @match        https://*.hamster.yandex.uz/search/*
// @match        https://*.hamster.yandex.eu/search/*
// @grant        none
// ==/UserScript==

(function() {
    'use strict';

    const server = "http://bjarne.man.yp-c.yandex.net:13938";

    var count = 0;
    var mapping = {};
    var messageListenerRegistered = false;

    var registerMessageListener = function() {
        window.addEventListener("message", function(event) {
            if (event.origin != server) return;
            event.source.postMessage(mapping[event.data], server);
        }, false);
        messageListenerRegistered = true;
    };

    var callback = function(mutationList, observer) {
        var kukaInfo = document.getElementsByClassName("internal-table__head-inner");
        for (var i = 0; i < kukaInfo.length; i++) {
            if (kukaInfo[i].innerText === "TPbDocHitsSerializedBase64") {
                var content = kukaInfo[i].parentNode.parentNode.nextSibling.firstChild.firstChild;
                if (content.childElementCount > 0) {
                    continue; // already processed
                }
                if (!messageListenerRegistered) {
                    registerMessageListener();
                }
                content.appendChild(document.createElement("br"));
                var link = document.createElement("a");
                var key = "#opener=" + count;
                mapping[key] = content.innerText;
                count += 1;
                link.setAttribute("href", server + "/dochits" + key);
                link.setAttribute("target", "_blank");
                link.setAttribute("rel", "opener");
                link.innerText = "Open a new tab in external interface";
                content.appendChild(link);
            }
        }
    };

    var observer = new MutationObserver(callback);
    observer.observe(document.body, {childList: true, subtree: true});
})();
