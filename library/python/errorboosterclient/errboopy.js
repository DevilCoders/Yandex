// ==UserScript==
// @name         errboopy
// @namespace    errorbooster
// @version      0.1
// @description  Alternative layout for ErrorBooster items page
// @author       idlesign
// @match        https://error.yandex-team.ru/projects/*/errors/*/items*
// @require      http://code.jquery.com/jquery-3.4.1.min.js
// @require      https://cdn.jsdelivr.net/npm/bootstrap@5.0.1/dist/js/bootstrap.bundle.min.js
// @require      https://cdn.jsdelivr.net/npm/uuid@latest/dist/umd/uuidv4.min.js

// ==/UserScript==

(function() {
    'use strict';


    window.setTimeout(function() {
        alter({
            debug: true,
        });
    }, 3000);


})();


function alter(options){

    options = $.extend({

        debug: false,

    }, options)

    console.log('errboopy is triggered')

    $(
        '<style>' +
            '#workarea {font-family: Rubik, "Avenir Next", "Helvetica Neue", sans-serif}' +
            '.collapser {border-bottom: 1px dashed; cursor: pointer}' +
        '</style>' +

        '<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.0.1/dist/css/bootstrap.min.css" ' +
        'rel="stylesheet" integrity="sha384-+0n0xVW2eSR5OomGNYDnhzAbDsOXxcvSN1TPprVMTNDbiYZCxYbOOl7+AMvyTG2x" crossorigin="anonymous">'

    ).appendTo('head')

    let $dialog = $(

            '<div id="ebp-dialog" class="modal fade" style="display: none"><div class="modal-dialog modal-xl modal-dialog-scrollable">' +
                '<div class="modal-content">' +
                    '<div class="modal-header"><h5 id="areaTitle" class="modal-title"></h5><button type="button" class="btn-close" data-bs-dismiss="modal"></button></div>' +
                    '<div class="modal-body fs-4" id="workarea"></div>' +
            '</div></div></div>'

        ).appendTo('body'),
        $container = $('#workarea'),
        areaTitle = '',
        eventCount = 0;

    $.each($('tr', '.table-container__table'), function(){

        let text = $(this).text();

        if (text[0] == '{'){

            let info = JSON.parse(text);

            // console.log(info);

            eventCount++;
            areaTitle = info.originalMessage;

            $container.append($(
                '<div class="bg-light shadow mb-4 p-2 rounded">' +
                    '<div class="fs-6 mb-3">' +
                        '<small class="text-muted">' + info.dateTime + ' &nbsp; ID события: ' + info.additional.eventid + '</small>' +
                        '<span class="badge bg-primary me-2 float-end">' + info.environment + '</span>' +
                        '<span class="badge bg-secondary me-2 float-end">вер. ' + info.version + '</span>' +
                        '<span class="badge bg-light text-dark me-2 float-end">' + info.host + '</span>' +
                    '</div><ul>' +
                    '<li>' + getAccordion(
                        'Цепочка событий',
                        info.additional.breadcrumbs,
                        function(item) {
                            return [
                                '<span class="text-nowrap">' + item.timestamp.replace('T', ' ') + '</span>' +
                                '<span class="badge bg-secondary mx-2">' + item.category + ': ' + item.type + '</span> ' +
                                item.message,

                                '<code class="fs-6">' + item.message + '</code>' +
                                '<div class="mt-2 fs-6"><b>Данные</b><br><code class="fs-6">' + getMapped(item.data) + '</code></div>'
                            ]
                        }
                    ) + '</li>' +
                    '<li>' + getAccordion(
                        'Трассировка',
                        info.additional.vars,
                        function(item) {return [item.loc, getMapped(item.vars)]}
                    ) + '</li>' +
                    '<li>' + getCollapser(
                        'Окружение',

                        getMapped(info.additional.contexts) +
                        getMapped(info.additional.extra) +
                        getMapped(info.additional.modules)

                    ) + '</li>' +
                '</ul></div>'
            ));
        }
    });

    $('#areaTitle').html('<sup>' + eventCount + '</sup> ' + areaTitle);

    let modal = new bootstrap.Modal(document.getElementById('ebp-dialog'));
    modal.show();

}


function getAccordionItem(title, body) {

    let uid = uuidv4(),
        item = (
            '<div class="accordion-item">' +
                '<h2 class="accordion-header">' +
                '<button class="accordion-button collapsed" type="button" data-bs-toggle="collapse" data-bs-target="#ai-' + uid + '">' +
                title.substring(0, 200) +
                '</button></h2>' +
                '<div id="ai-' + uid + '" class="accordion-collapse collapse">' +
                    '<div class="accordion-body">' + body + '</div>' +
            '</div></div>'
        )

    return item;
}

function getAccordion(title, items, itemCallback){

    let elUid = uuidv4(),
        accordion = [
            '<div class="accordion mt-3">'
        ];

    items = items || [];

    items.forEach(function(item){
        accordion.push(getAccordionItem.apply(null, itemCallback(item)))
    });
    accordion.push('</div>');

    return getCollapser(title + ' <sup>' + items.length + '</sup>', accordion.join('\n'))

}

function getCollapser(title, body) {
    let elUid = uuidv4();
    return (
        '<div class="my-2 fs-6 text-muted"><span class="collapser" data-bs-toggle="collapse" href="#cl-' + elUid + '">' +
        '<strong>' + title + '</strong></span>' +
        '<div class="collapse" id="cl-' + elUid + '">' + body + '</div></div>'
    );

}

function getMapped(obj) {

    let lines = [
        '<table class="table table-bordered table-hover table-sm mt-3" style="table-layout: fixed">'
    ];

    obj = obj || {};

    for (let [key, val] of Object.entries(obj)) {
        if (typeof val === 'object' && val !== null) {
            val = getMapped(val);
        } else {
            val = $('<div>').text(val).html(); // escape
        }
        lines.push('<tr><td width="20%"><code>' + key + '</code></td><td class="text-break">' + val + '</td></tr>');
    }

    lines.push('</table>');

    return lines.join('\n');
}
