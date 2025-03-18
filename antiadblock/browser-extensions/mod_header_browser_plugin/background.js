let currentProfile;
let requestsCounter = {};

function loadSelectedProfile_() {
  let appendMode = false;
  let headers = [];
  let respHeaders = [];
  if (localStorage.profile_aab) {
    const selectedProfile = JSON.parse(localStorage.profile_aab);

    function filterEnabledHeaders_(headers) {
      const output = [];
      for (const header of headers) {
        // Overrides the header if it is enabled and its name is not empty.
        if (header.enabled && header.name) {
          output.push({ name: header.name, value: header.value });
        }
      }
      return output;
    }
    appendMode = selectedProfile.appendMode;
    headers = filterEnabledHeaders_(selectedProfile.headers);
    respHeaders = filterEnabledHeaders_(selectedProfile.respHeaders);
  }
  return {
    appendMode: appendMode,
    headers: headers,
    respHeaders: respHeaders,
  };
}

function modifyHeader(source, dest) {
  if (!source.length) {
    return;
  }
  const indexMap = {};
  for (const index in dest) {
    const header = dest[index];
    indexMap[header.name.toLowerCase()] = index;
  }
  for (const header of source) {
    const index = indexMap[header.name.toLowerCase()];
    if (index !== undefined) {
      if (!currentProfile.appendMode) {
        dest[index].value = header.value;
      } else if (currentProfile.appendMode == 'comma') {
        if (dest[index].value) {
          dest[index].value += ',';
        }
        dest[index].value += header.value;
      } else {
        dest[index].value += header.value;
      }
    } else {
      if (header.value.indexOf('{cnt}') != -1) {
        if (requestsCounter[header.name] === undefined) {
          requestsCounter[header.name] = 0;
        } else {
          requestsCounter[header.name] += 1
        }
        dest.push({ name: header.name, value: header.value.replace('{cnt}', '') + requestsCounter[header.name].toString()});
      } else {
        dest.push({ name: header.name, value: header.value });
      }
      indexMap[header.name.toLowerCase()] = dest.length - 1;
    }
  }
}


function getAllUrlParams(url) {

  // get query string from url (optional) or window
  var queryString = url ? url.split('?')[1] : window.location.search.slice(1);

  // we'll store the parameters here
  var obj = {};

  // if query string exists
  if (queryString) {

    // stuff after # is not part of query string, so get rid of it
    queryString = queryString.split('#')[0];

    // split our query string into its component parts
    var arr = queryString.split('&');

    for (var i = 0; i < arr.length; i++) {
      // separate the keys and the values
      var a = arr[i].split('=');

      // set parameter name and value (use 'true' if empty)
      var paramName = a[0];
      var paramValue = typeof (a[1]) === 'undefined' ? true : a[1];

      // (optional) keep case consistent
      paramName = paramName.toLowerCase();
      if (typeof paramValue === 'string') paramValue = paramValue.toLowerCase();

      // if the paramName ends with square brackets, e.g. colors[] or colors[2]
      if (paramName.match(/\[(\d+)?\]$/)) {

        // create key if it doesn't exist
        var key = paramName.replace(/\[(\d+)?\]/, '');
        if (!obj[key]) obj[key] = [];

        // if it's an indexed array e.g. colors[2]
        if (paramName.match(/\[\d+\]$/)) {
          // get the index value and add the entry at the appropriate position
          var index = /\[(\d+)\]/.exec(paramName)[1];
          obj[key][index] = paramValue;
        } else {
          // otherwise add the value to the end of the array
          obj[key].push(paramValue);
        }
      } else {
        // we're dealing with a string
        if (!obj[paramName]) {
          // if it doesn't exist, create property
          obj[paramName] = paramValue;
        } else if (obj[paramName] && typeof obj[paramName] === 'string'){
          // if property does exist and it's a string, convert it to an array
          obj[paramName] = [obj[paramName]];
          obj[paramName].push(paramValue);
        } else {
          // otherwise add the property
          obj[paramName].push(paramValue);
        }
      }
    }
  }
  return obj;
}


function onBeforeRequestHandler_(details) {

  if (details.url.indexOf('//ya.ru/clear') != -1) {
    localStorage.removeItem('profile_aab');
    requestsCounter = {};

    return {
      redirectUrl: 'https://ya.ru/'
    };
  }

  if (details.url.indexOf('//ya.ru/add') != -1) {
    console.log(details.url);
    let selectedProfile = {
      appendMode: false,
      headers: [],
      respHeaders: []
    };
    params = getAllUrlParams(details.url);

    if (localStorage.profile_aab) {
      selectedProfile = JSON.parse(localStorage.profile_aab);
    }

    for (var key in params) {
      selectedProfile.headers.push({
        name: key, value: params[key], enabled: true
      })
    }

    localStorage.profile_aab = JSON.stringify(selectedProfile);
    return {
      redirectUrl: 'https://ya.ru/'
    };
  }
}

function modifyRequestHeaderHandler_(details) {
  currentProfile = loadSelectedProfile_();
  if (currentProfile) {
    modifyHeader(currentProfile.headers, details.requestHeaders);
  }
  return { requestHeaders: details.requestHeaders };
}

function modifyResponseHeaderHandler_(details) {
  if (currentProfile) {
    const responseHeaders = JSON.parse(JSON.stringify(details.responseHeaders));
    modifyHeader(currentProfile.respHeaders, responseHeaders);
    if (
      JSON.stringify(responseHeaders) != JSON.stringify(details.responseHeaders)
    ) {
      return { responseHeaders: responseHeaders };
    }
  }
}

chrome.webRequest.onBeforeRequest.addListener(
  onBeforeRequestHandler_,
  { urls: ['<all_urls>'] },
  ['blocking']
);

function getChromeVersion() {
  let pieces = navigator.userAgent.match(
    /Chrom(?:e|ium)\/([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)/
  );
  if (pieces == null || pieces.length != 5) {
    return undefined;
  }
  pieces = pieces.map(piece => parseInt(piece, 10));
  return {
    major: pieces[1],
    minor: pieces[2],
    build: pieces[3],
    patch: pieces[4]
  };
}

const CHROME_VERSION = getChromeVersion();
const requiresExtraHeaders = CHROME_VERSION && CHROME_VERSION.major >= 72;

chrome.webRequest.onBeforeSendHeaders.addListener(
  modifyRequestHeaderHandler_,
  { urls: ['<all_urls>'] },
  requiresExtraHeaders
    ? ['requestHeaders', 'blocking', 'extraHeaders']
    : ['requestHeaders', 'blocking']
);

chrome.webRequest.onHeadersReceived.addListener(
  modifyResponseHeaderHandler_,
  { urls: ['<all_urls>'] },
  requiresExtraHeaders
    ? ['responseHeaders', 'blocking', 'extraHeaders']
    : ['responseHeaders', 'blocking']
);
