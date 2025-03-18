/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @module hitLogger */

// eslint-disable-next-line strict
window.adbCustomFiltersData = {};

import {SpecialSubscription} from
  "../adblockpluscore/lib/subscriptionClasses.js";
import {extractHostFromFrame} from "./url.js";
import {EventEmitter} from "../adblockpluscore/lib/events.js";
import {filterStorage} from "../adblockpluscore/lib/filterStorage.js";
import {port} from "./messaging.js";
// eslint-disable-next-line max-len
import {AllowingFilter, ElemHideException, Filter, ElemHideFilter} from "../adblockpluscore/lib/filterClasses.js";
import {contentTypes} from "../adblockpluscore/lib/contentTypes.js";
import {checkAllowlisted} from "./allowlisting.js";

export let nonRequestTypes = [
  "DOCUMENT", "ELEMHIDE", "SNIPPET", "GENERICBLOCK", "GENERICHIDE", "CSP"
];

let eventEmitter = new EventEmitter();

function adbCustomLog(filter, request)
{
  if (filter && typeof filter === "object")
  {
    const filterInfo = getFilterInfo(filter);
    // eslint-disable-next-line max-len
    if (!filterInfo.whitelisted)
    {
      // eslint-disable-next-line max-len
      const data = Object.assign(filterInfo, request);
      data.text = data.text.replace(/[',"]/g, '\\"');
      if (!window.adbCustomFiltersData[data.text])
      {
        // eslint-disable-next-line max-len
        window.adbCustomFiltersData[data.text] = [data];
        insertScriptOnPage();
        // eslint-disable-next-line max-len
      }
      // eslint-disable-next-line max-len
      else if (!window.adbCustomFiltersData[data.text].some(item => JSON.stringify(item) === JSON.stringify(data)))
      {
        // eslint-disable-next-line max-len
        window.adbCustomFiltersData[data.text].push(data);
        insertScriptOnPage();
      }
    }
  }
}

function insertScriptOnPage()
{
  // eslint-disable-next-line max-len
  // хак со вставкой инлайнового скрипта на страницу, другим способом изменить window не получится.
  chrome.tabs.executeScript({
    code:
        `
          adbFilterScript = null;
          adbFilterScript = document.createElement('script');
          adbFilterScript.innerHTML = 'window.adbFiltersData = ${JSON.stringify(window.adbCustomFiltersData)};'
          document.body.appendChild(adbFilterScript);
        `
  })
      .then(() => {})
      .catch(() => {});
}

function getFilterInfo(filter)
{
  if (!filter)
    return null;

  let userDefined = false;
  let subscriptionTitle = null;

  for (let subscription of filterStorage.subscriptions(filter.text))
  {
    if (!subscription.disabled)
    {
      if (subscription instanceof SpecialSubscription)
        userDefined = true;
      else
        subscriptionTitle = subscription.title;
    }
  }

  return {
    text: filter.text,
    whitelisted: filter instanceof AllowingFilter ||
        filter instanceof ElemHideException,
    userDefined,
    subscription: subscriptionTitle
  };
}

/**
 * @namespace
 * @static
 */
export let HitLogger = {
  /**
   * Adds a listener for requests, filter hits etc related to the tab.
   *
   * Note: Calling code is responsible for removing the listener again,
   *       it will not be automatically removed when the tab is closed.
   *
   * @param {number} tabId
   * @param {function} listener
   */
  addListener: eventEmitter.on.bind(eventEmitter),

  /**
   * Removes a listener for the tab.
   *
   * @param {number} tabId
   * @param {function} listener
   */
  removeListener: eventEmitter.off.bind(eventEmitter),

  /**
   * Checks whether a tab is being inspected by anything.
   *
   * @param {number} tabId
   * @return {boolean}
   */
  hasListener: eventEmitter.hasListeners.bind(eventEmitter)
};

/**
 * Logs a request associated with a tab or multiple tabs.
 *
 * @param {number[]} tabIds
 *   The tabIds associated with the request
 * @param {Object} request
 *   The request to log
 * @param {string} request.url
 *   The URL of the request
 * @param {string} request.type
 *  The request type
 * @param {string} request.docDomain
 *  The hostname of the document
 * @param {boolean} request.thirdParty
 *   Whether the origin of the request and document differs
 * @param {?string} request.sitekey
 *   The active sitekey if there is any
 * @param {?boolean} request.specificOnly
 *   Whether generic filters should be ignored
 * @param {?BlockingFilter} filter
 *  The matched filter or null if there is no match
 */
export function logRequest(tabIds, request, filter)
{
  adbCustomLog(filter, request);

  for (let tabId of tabIds)
    eventEmitter.emit(tabId, request, filter);
}

function logHiddenElements(page, frame, selectors, filters, docDomain)
{
  // if (HitLogger.hasListener(page.id))
  // {
  for (let subscription of filterStorage.subscriptions())
  {
    if (subscription.disabled)
      continue;

    for (let text of subscription.filterText())
    {
      let filter = Filter.fromText(text);

      // We only know the exact filter in case of element hiding emulation.
      // For regular element hiding filters, the content script only knows
      // the selector, so we have to find a filter that has an identical
      // selector and is active on the domain the match was reported from.
      let isActiveElemHideFilter = filter instanceof ElemHideFilter &&
          selectors.includes(filter.selector) &&
          filter.isActiveOnDomain(docDomain);

      if (isActiveElemHideFilter || filters.includes(text))
      {
        // For generic filters we need to additionally check whether
        // they were not applied to the page due to an exception rule.
        if (filter.isGeneric())
        {
          let specificOnly = checkAllowlisted(
              page, frame, null,
              contentTypes.GENERICHIDE
          );
          if (specificOnly)
            continue;
        }

        adbCustomLog(filter, {
          type: "ELEMHIDE",
          docDomain,
          url: docDomain
        });

        eventEmitter.emit(page.id, {type: "ELEMHIDE", docDomain}, filter);
      }
    }
  }
  // }
}

/**
 * Logs an allowing filter that disables (some kind of)
 * blocking for a particular document.
 *
 * @param {number}       tabId     The tabId the allowlisting is active for
 * @param {string}       url       The url of the allowlisted document
 * @param {number}       typeMask  The bit mask of allowing types checked
 *                                 for
 * @param {string}       docDomain The hostname of the parent document
 * @param {AllowingFilter} filter  The matched allowing filter
 */
export function logAllowlistedDocument(tabId, url, typeMask, docDomain, filter)
{
  if (HitLogger.hasListener(tabId))
  {
    for (let type of nonRequestTypes)
    {
      if (typeMask & filter.contentType & contentTypes[type])
        eventEmitter.emit(tabId, {url, type, docDomain}, filter);
    }
  }
}

ext.pages.onLoading.addListener(() =>
{
  window.adbCustomFiltersData = {};
});

/**
 * Logs active element hiding filters for a tab.
 *
 * @event "hitLogger.traceElemHide"
 * @property {string[]} selectors  The selectors of applied ElemHideFilters
 * @property {string[]} filters    The text of applied ElemHideEmulationFilters
 */
port.on("hitLogger.traceElemHide", (message, sender) =>
{
  logHiddenElements(
    sender.page, sender.frame, message.selectors, message.filters,
    extractHostFromFrame(sender.frame)
  );
});
