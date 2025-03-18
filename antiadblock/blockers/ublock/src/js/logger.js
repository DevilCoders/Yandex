/*******************************************************************************

    uBlock Origin - a browser extension to block requests.
    Copyright (C) 2015-present Raymond Hill

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see {http://www.gnu.org/licenses/}.

    Home: https://github.com/gorhill/uBlock
*/

'use strict';

/******************************************************************************/

window.adbCustomFiltersData = {};
window.lastUsedTabId = null;

function insertScriptOnPage()
{
    // eslint-disable-next-line max-len
    // хак со вставкой инлайнового скрипта на страницу, другим способом изменить window не получится.
    chrome.tabs.executeScript({
        code: `
            adbFilterScript = null;
            adbFilterScript = document.createElement('script');
            adbFilterScript.innerHTML = 'window.adbFiltersData = ${JSON.stringify(window.adbCustomFiltersData)};';
            document.body.appendChild(adbFilterScript);
        `
    });
}

function adbCustomLog(details) {
    if (details.filter) {
        if (details.tabId !== window.lastUsedTabId) {
            window.lastUsedTabId = details.tabId;
            window.adbCustomFiltersData = {};
        }

        const data = JSON.parse(JSON.stringify(details));
        // при копировании объекта в window страницы падает строка в которой есть двойные ковычки
        // пример: rule: "div[aaa="test"]"
        data.filter.raw = data.filter.raw.replace(/"/g, '\\"');
        if (!window.adbCustomFiltersData[details.filter.raw])
        {
            // eslint-disable-next-line max-len
            window.adbCustomFiltersData[data.filter.raw] = [data];
            insertScriptOnPage();
            // eslint-disable-next-line max-len
        }
        // eslint-disable-next-line max-len
        else if (!window.adbCustomFiltersData[data.filter.raw].some(item => JSON.stringify(item) === JSON.stringify(details))) {
            // eslint-disable-next-line max-len
            window.adbCustomFiltersData[data.filter.raw].push(data);
            insertScriptOnPage();
        }
    }
}

let buffer = null;
let lastReadTime = 0;
let writePtr = 0;

// After 30 seconds without being read, a buffer will be considered
// unused, and thus removed from memory.
const logBufferObsoleteAfter = 30 * 1000;

const janitor = ( ) => {
    if (
        buffer !== null &&
        lastReadTime < (Date.now() - logBufferObsoleteAfter)
    ) {
        logger.enabled = false;
        buffer = null;
        writePtr = 0;
        logger.ownerId = undefined;
        vAPI.messaging.broadcast({ what: 'loggerDisabled' });
    }
    if ( buffer !== null ) {
        vAPI.setTimeout(janitor, logBufferObsoleteAfter);
    }
};

const boxEntry = function(details) {
    if ( details.tstamp === undefined ) {
        details.tstamp = Date.now();
    }
    return JSON.stringify(details);
};

const logger = {
    enabled: true,
    ownerId: undefined,
    writeOne: function(details) {
        adbCustomLog(details);
        if ( buffer === null ) { return; }
        const box = boxEntry(details);
        if ( writePtr === buffer.length ) {
            buffer.push(box);
        } else {
            buffer[writePtr] = box;
        }
        writePtr += 1;
    },
    readAll: function(ownerId) {
        this.ownerId = ownerId;
        if ( buffer === null ) {
            this.enabled = true;
            buffer = [];
            vAPI.setTimeout(janitor, logBufferObsoleteAfter);
        }
        const out = buffer.slice(0, writePtr);
        writePtr = 0;
        lastReadTime = Date.now();
        return out;
    },
};

/******************************************************************************/

export default logger;

/******************************************************************************/
