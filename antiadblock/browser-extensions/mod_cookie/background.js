const ADD = '+';
const DEL = '-';
const BLOCK = '^';
const UNBLOCK = '!^';


function onBeforeRequestHandler_(details) {
    if (details.url.indexOf('//ya.ru/cookies') != -1) {
        console.log(details.url);
        const params = getAllUrlParams(details.url);

        for (host in params) {
            const url = 'https://' + host;
            const host_parts = host.split('.');
            const domain = `.${host_parts[host_parts.length - 2]}.${host_parts[host_parts.length - 1]}`

            for (let cookie in params[host][DEL]) {
                chrome.cookies.remove({ url: url, name: cookie });
            }

            for (let cookie in params[host][ADD]) {
                const value = params[host][ADD][cookie];
                chrome.cookies.set({ url: url, domain: domain, name: cookie, value: value, secure: true });
            }

            let blockedCookies = {};
            if (('blockedCookies' in localStorage)) {
                try {
                    blockedCookies = JSON.parse(localStorage.blockedCookies);
                } catch {
                }
            }
            if (!(host in blockedCookies)) {
                blockedCookies[host] = [];
            }

            // Unblock cookies
            blockedCookies[host] = blockedCookies[host].filter(e => { return !(params[host][UNBLOCK].some(cookie => e === cookie)); });
            // Deduplicate
            blockedCookies[host] = blockedCookies[host].filter(e => { return !(params[host][BLOCK].some(cookie => e === cookie)); });
            for (const cookie of params[host][BLOCK]) {
                blockedCookies[host].push(cookie);
            }
            localStorage.blockedCookies = JSON.stringify(blockedCookies);
        }

        return {
            redirectUrl: 'https://ya.ru/'
        };
    }
}

function getAllUrlParams(url) {
    let queryString = url ? url.split('?')[1] : window.location.search.slice(1);
    let obj = {};
    if (queryString) {
        queryString = queryString.split('#')[0];
        let arr = queryString.split('&');
        for (let i = 0; i < arr.length; i++) {
            const a = arr[i].split('=');
            const paramName = a[0].toLowerCase();
            const paramValue = typeof (a[1]) === 'undefined' ? '1' : a[1].toLowerCase();
            const [op, host, name] = paramName.split('::');

            console.log(op, host, name, paramValue);
            if (!(host in obj)) {
                obj[host] = {};
                obj[host][ADD] = {};
                obj[host][DEL] = {};
                obj[host][BLOCK] = [];
                obj[host][UNBLOCK] = [];
            }
            if (Array.isArray(obj[host][op])) {
                obj[host][op].push(name);
            } else {
                obj[host][op][name] = paramValue;
            }
        }
    }
    return obj;
}

function modifyRequestHeaderHandler_(details) {
    if (localStorage.blockedCookies) {
        let blockedCookies = getBlockedCookies(details.url)
        if (blockedCookies != null) {
            removeCookies(details.requestHeaders, blockedCookies.cookies)
            const url = `https://${blockedCookies.host}`
            for (const cookie of blockedCookies.cookies) {
                chrome.cookies.remove({ url: url, name: cookie });
            }
        }
    }
    return { requestHeaders: details.requestHeaders }
}

function getBlockedCookies(url) {
    let blockedCookies = JSON.parse(localStorage.blockedCookies);
    for (let host in blockedCookies) {
        if (url.indexOf(host) != -1) {
            return { host: host, cookies: blockedCookies[host] };
        }
    }
    return null
}

function removeCookies(headers, cookies) {
    for (var i = 0; i < headers.length; ++i) {
        if (headers[i].name === 'Cookie') {
            headers[i].value += ';';
            for (const cookie of cookies) {
                let regex = new RegExp(`${cookie}=.+?;`)
                headers[i].value = headers[i].value.replace(regex, '');
            }
            break;
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
