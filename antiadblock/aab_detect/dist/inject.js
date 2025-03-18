(function(e, a) { for(var i in a) e[i] = a[i]; }(window, /******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, {
/******/ 				configurable: false,
/******/ 				enumerable: true,
/******/ 				get: getter
/******/ 			});
/******/ 		}
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 40);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.BLOCKERS = exports.BlockedResourceType = void 0;
var BlockedResourceType;
(function (BlockedResourceType) {
    BlockedResourceType[BlockedResourceType["ELEMENT"] = 0] = "ELEMENT";
    BlockedResourceType[BlockedResourceType["NETWORK"] = 1] = "NETWORK";
    BlockedResourceType[BlockedResourceType["IN_IFRAME"] = 2] = "IN_IFRAME";
    BlockedResourceType[BlockedResourceType["INSTANT"] = 3] = "INSTANT";
    BlockedResourceType[BlockedResourceType["EXCEPTION"] = 4] = "EXCEPTION";
    BlockedResourceType[BlockedResourceType["FAKE"] = 5] = "FAKE";
    BlockedResourceType[BlockedResourceType["UNKNOWN"] = 6] = "UNKNOWN";
})(BlockedResourceType = exports.BlockedResourceType || (exports.BlockedResourceType = {}));
var BLOCKERS;
(function (BLOCKERS) {
    BLOCKERS["UNKNOWN"] = "UNKNOWN";
    BLOCKERS["NOT_BLOCKED"] = "NOT_BLOCKED";
    BLOCKERS["ADBLOCK"] = "ADBLOCK";
    BLOCKERS["ADBLOCKPLUS"] = "ADBLOCKPLUS";
    BLOCKERS["ADMUNCHER"] = "ADBLOCKPLUS";
    BLOCKERS["ADGUARD"] = "ADGUARD";
    BLOCKERS["UBLOCK"] = "UBLOCK";
    BLOCKERS["GHOSTERY"] = "GHOSTERY";
    BLOCKERS["UK"] = "UK";
    BLOCKERS["FF_PRIVATE"] = "FF_PRIVATE";
    BLOCKERS["KIS"] = "KIS";
    BLOCKERS["EXPERIMENT"] = "EXPERIMENT";
    BLOCKERS["BRAVE"] = "BRAVE";
    BLOCKERS["UCBROWSER"] = "UCBROWSER";
    BLOCKERS["ADBLOCK_BROWSER"] = "ADBLOCK_BROWSER";
    BLOCKERS["OPERA_BROWSER"] = "OPERA_BROWSER";
    BLOCKERS["DNS"] = "DNS";
})(BLOCKERS = exports.BLOCKERS || (exports.BLOCKERS = {}));

/***/ }),
/* 1 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.TestBase = void 0;
var blockers_1 = __webpack_require__(0);
var TestBase = function () {
    function TestBase() {}
    TestBase.prototype.light = function (blockedResource) {
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    TestBase.prototype.heavy = function (callback, blockedResource) {
        callback({
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        });
    };
    TestBase.prototype.teardown = function () {};
    return TestBase;
}();
exports.TestBase = TestBase;

/***/ }),
/* 2 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.config = void 0;
var adblock_1 = __webpack_require__(45);
var CONFIG_MACROS_1 = __webpack_require__(46);
var adblockConfig = CONFIG_MACROS_1.M;
var defaultConfig = {
    pid: '',
    encode: {
        key: ''
    },
    replaceClasses: [],
    detect: {
        links: [],
        custom: [],
        iframes: []
    },
    cookieMatching: {
        publisherTag: '',
        publisherKey: '',
        types: [],
        type: adblock_1.AdblockCookieMatchingType.doNotMatch,
        redirectUrl: ['//an.yand', 'ex.ru/map', 'uid/'].join(''),
        imageUrl: '/static/img/logo.gif/',
        cryptedUidUrl: 'https://http-check-headers.yandex.ru',
        cryptedUidCookie: 'crookie',
        cryptedUidTTL: 14 * 24
    },
    additionalParams: {},
    cookieTTL: 14 * 24,
    extuidCookies: [],
    debug: false,
    force: false,
    disableShadow: false,
    forcecry: {
        enabled: false,
        expires: 0,
        percent: 0
    },
    treeProtection: {
        enabled: false
    },
    countToXhr: false,
    blockToIframeSelectors: [],
    pcodeDebug: false
};
if (false) {
    defaultConfig.fn = {
        encodeCSS: function encodeCSS(className) {
            return className;
        },
        encodeUrl: function encodeUrl(url) {
            return url;
        },
        decodeUrl: function decodeUrl(url) {
            return url;
        },
        isEncodedUrl: function isEncodedUrl(url) {
            return false;
        }
    };
}
function isString() {
    return typeof adblockConfig === 'string';
}
exports.config = isString() ? defaultConfig : adblockConfig;

/***/ }),
/* 3 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getUserAgent = void 0;
function getUserAgent(win) {
    if (win === void 0) {
        win = window;
    }
    try {
        var navigator = win.navigator || {};
        return navigator.userAgent || '';
    } catch (e) {
        return '';
    }
}
exports.getUserAgent = getUserAgent;

/***/ }),
/* 4 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.checkNativeCode = void 0;
function checkNativeCode(fn) {
    if (!fn || !fn.toString) {
        return false;
    }
    var fnCode = fn.toString();
    return (/\[native code\]/.test(fnCode) || /\/\* source code not available \*\//.test(fnCode)
    );
}
exports.checkNativeCode = checkNativeCode;

/***/ }),
/* 5 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getInternetExplorerVersion = void 0;
var detectie = __webpack_require__(93);
var getInternetExplorerVersion = function getInternetExplorerVersion(win) {
    if (win === void 0) {
        win = window;
    }
    var detectedInternetExplorerVersion = detectie(win);
    return typeof detectedInternetExplorerVersion === 'boolean' ? -1 : detectedInternetExplorerVersion;
};
exports.getInternetExplorerVersion = getInternetExplorerVersion;

/***/ }),
/* 6 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.removeElements = exports.removeElement = exports.createTestIframe = exports.createDivWithContent = void 0;
var containerStyle = ['position: absolute !important;', 'top: -100px;', 'left: -100px;', 'height: 75px;', 'width: 75px;', 'overflow: hidden;'].join(' ');
function createDivWithContent(content) {
    var element = document.createElement('div');
    element.innerHTML = content;
    element.style.cssText = containerStyle;
    document.body.insertBefore(element, null);
    return element;
}
exports.createDivWithContent = createDivWithContent;
function createTestIframe(width, height, src) {
    if (src === void 0) {
        src = '';
    }
    var elementStyle = ['display: block;', 'position: absolute !important;', 'top: -100px;', 'left: -1000px;', 'overflow: hidden;'].join(' ');
    var element = document.createElement('iframe');
    element.setAttribute('width', width.toString(10) + 'px');
    element.setAttribute('height', height.toString(10) + 'px');
    if (src) {
        element.setAttribute('src', src);
    }
    element.style.cssText = elementStyle;
    document.body.insertBefore(element, null);
    return element;
}
exports.createTestIframe = createTestIframe;
function removeElement(element) {
    if (element && element.parentNode) {
        element.parentNode.removeChild(element);
    }
}
exports.removeElement = removeElement;
function removeElements(elements) {
    for (var i = 0; i < elements.length; i++) {
        removeElement(elements[i]);
    }
}
exports.removeElements = removeElements;

/***/ }),
/* 7 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.map = void 0;
var map = function map(array, callback, thisArg) {
    var result = new Array(array.length);
    for (var index = 0; index < array.length; index++) {
        result[index] = callback.call(thisArg, array[index], index, array);
    }
    return result;
};
exports.map = map;

/***/ }),
/* 8 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isFunction = void 0;
var getInternalClass_1 = __webpack_require__(18);
function isFunction(value) {
    return typeof value === 'function' || getInternalClass_1.getInternalClass(value) === 'Function';
}
exports.isFunction = isFunction;

/***/ }),
/* 9 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsSafari = void 0;
var getUserAgent_1 = __webpack_require__(3);
var getIsSafari = function getIsSafari(win) {
    if (win === void 0) {
        win = window;
    }
    var userAgent = getUserAgent_1.getUserAgent(win).toLowerCase();
    if (userAgent.indexOf('android') > -1) {
        return false;
    }
    var keys = userAgent.replace(/\(.+?\)/gi, '').split(' ').map(function (pair) {
        return pair.trim().split('/')[0];
    }).filter(function (key) {
        return key && key !== 'mobile';
    });
    return keys.length === 4 && keys[0] === 'mozilla' && keys[1] === 'applewebkit' && keys[2] === 'version' && keys[3] === 'safari';
};
exports.getIsSafari = getIsSafari;

/***/ }),
/* 10 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsIOS = void 0;
var isUCBrowser_1 = __webpack_require__(32);
function getIsIOS(win) {
    if (win === void 0) {
        win = window;
    }
    var userAgent = win.navigator.userAgent.toLowerCase();
    return (/ipad|iphone|ipod/.test(userAgent) && !win.MSStream && !isUCBrowser_1.isUCBrowser(win)
    );
}
exports.getIsIOS = getIsIOS;

/***/ }),
/* 11 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getShadowRootText = void 0;
function getShadowRootText() {
    var shadowRoot = Object.getOwnPropertyDescriptor(window.Element.prototype, 'shadowRoot');
    if (shadowRoot && shadowRoot.get) {
        var rootText = shadowRoot.get.toString();
        if (rootText.indexOf('[native code]') === -1) {
            return rootText;
        }
    }
    return '';
}
exports.getShadowRootText = getShadowRootText;

/***/ }),
/* 12 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.some = void 0;
var some = function some(array, callback) {
    for (var index = 0; index < array.length; index++) {
        if (callback(array[index], index, array)) {
            return true;
        }
    }
    return false;
};
exports.some = some;

/***/ }),
/* 13 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isInIframe = void 0;
function isInIframe(win) {
    return win.top !== win.self;
}
exports.isInIframe = isInIframe;

/***/ }),
/* 14 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.addEquals = exports.cropEquals = exports.utf8Decode = exports.decodeUInt8String = exports.decode = exports.utf8Encode = exports.encodeUInt8String = exports.encode = void 0;
var base64alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=';
function encode(input, maxLength) {
    if (maxLength === void 0) {
        maxLength = 1e6;
    }
    var encoded = utf8Encode(input, maxLength * 3 / 4 | 0);
    return encodeUInt8String(encoded);
}
exports.encode = encode;
function encodeUInt8String(input) {
    var output = '';
    var i = 0;
    while (i < input.length) {
        var chr1 = input.charCodeAt(i++);
        var chr2 = input.charCodeAt(i++);
        var chr3 = input.charCodeAt(i++);
        var enc1 = chr1 >> 2;
        var enc2 = (chr1 & 3) << 4 | chr2 >> 4;
        var enc3 = (chr2 & 15) << 2 | chr3 >> 6;
        var enc4 = chr3 & 63;
        if (isNaN(chr2)) {
            enc3 = enc4 = 64;
        } else if (isNaN(chr3)) {
            enc4 = 64;
        }
        output = output + base64alphabet.charAt(enc1) + base64alphabet.charAt(enc2) + base64alphabet.charAt(enc3) + base64alphabet.charAt(enc4);
    }
    return output;
}
exports.encodeUInt8String = encodeUInt8String;
function utf8Encode(string, maxLength) {
    string = string.replace(/\r\n/g, '\n');
    var textInUtf = '';
    for (var n = 0; n < string.length; n++) {
        var c = string.charCodeAt(n);
        var nextChar = void 0;
        if (c < 128) {
            nextChar = String.fromCharCode(c);
        } else if (c > 127 && c < 2048) {
            nextChar = String.fromCharCode(c >> 6 | 192);
            nextChar += String.fromCharCode(c & 63 | 128);
        } else {
            nextChar = String.fromCharCode(c >> 12 | 224);
            nextChar += String.fromCharCode(c >> 6 & 63 | 128);
            nextChar += String.fromCharCode(c & 63 | 128);
        }
        if (textInUtf.length + nextChar.length > maxLength) {
            break;
        }
        textInUtf += nextChar;
    }
    return textInUtf;
}
exports.utf8Encode = utf8Encode;
function decode(input) {
    var output = decodeUInt8String(input);
    return utf8Decode(output);
}
exports.decode = decode;
function decodeUInt8String(input) {
    var output = [];
    var i = 0;
    input = input.replace(/[^A-Za-z0-9\-_=]/g, '');
    while (i < input.length) {
        var enc1 = base64alphabet.indexOf(input.charAt(i++));
        var enc2 = base64alphabet.indexOf(input.charAt(i++));
        var enc3 = base64alphabet.indexOf(input.charAt(i++));
        var enc4 = base64alphabet.indexOf(input.charAt(i++));
        var chr1 = enc1 << 2 | enc2 >> 4;
        var chr2 = (enc2 & 15) << 4 | enc3 >> 2;
        var chr3 = (enc3 & 3) << 6 | enc4;
        output.push(String.fromCharCode(chr1));
        if (enc3 !== 64) {
            output.push(String.fromCharCode(chr2));
        }
        if (enc4 !== 64) {
            output.push(String.fromCharCode(chr3));
        }
    }
    return output.join('');
}
exports.decodeUInt8String = decodeUInt8String;
function utf8Decode(textInUtf) {
    var result = [];
    var i = 0;
    while (i < textInUtf.length) {
        var c = textInUtf.charCodeAt(i);
        if (c < 128) {
            result.push(String.fromCharCode(c));
            i++;
        } else if (c > 191 && c < 224) {
            var c2 = textInUtf.charCodeAt(i + 1);
            result.push(String.fromCharCode((c & 31) << 6 | c2 & 63));
            i += 2;
        } else {
            var c2 = textInUtf.charCodeAt(i + 1);
            var c3 = textInUtf.charCodeAt(i + 2);
            result.push(String.fromCharCode((c & 15) << 12 | (c2 & 63) << 6 | c3 & 63));
            i += 3;
        }
    }
    return result.join('');
}
exports.utf8Decode = utf8Decode;
function cropEquals(base64) {
    return base64.replace(/=+$/, '');
}
exports.cropEquals = cropEquals;
function addEquals(base64) {
    while (base64.length % 4 !== 0) {
        base64 += '=';
    }
    return base64;
}
exports.addEquals = addEquals;

/***/ }),
/* 15 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isMobile = void 0;
var isMobile = function isMobile(win) {
    if (win === void 0) {
        win = window;
    }
    return (/Mobi|Android/i.test(win.navigator.userAgent)
    );
};
exports.isMobile = isMobile;

/***/ }),
/* 16 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.cssPrefixes = exports.prefixes = void 0;
exports.prefixes = ['', 'webkit', 'moz', 'o', 'ms'];
exports.cssPrefixes = ['', '-webkit-', '-ms-', '-moz-', '-o-'];

/***/ }),
/* 17 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __assign = void 0 && (void 0).__assign || function () {
    __assign = Object.assign || function (t) {
        for (var s, i = 1, n = arguments.length; i < n; i++) {
            s = arguments[i];
            for (var p in s) {
                if (Object.prototype.hasOwnProperty.call(s, p)) t[p] = s[p];
            }
        }
        return t;
    };
    return __assign.apply(this, arguments);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.deleteCookie = exports.getCookie = exports.setCookie = void 0;
var isDate_1 = __webpack_require__(42);
var setCookie = function setCookie(doc, key, value, attributes) {
    if (attributes === void 0) {
        attributes = {};
    }
    if (isDate_1.isDate(attributes.expires)) {
        attributes.expires = attributes.expires.toUTCString();
    }
    if (typeof attributes.path === 'undefined') {
        attributes.path = '/';
    }
    var stringifiedAttributes = '';
    for (var attributeName in attributes) {
        if (!attributes[attributeName]) {
            continue;
        }
        stringifiedAttributes += '; ' + attributeName;
        if (attributes[attributeName] === true) {
            continue;
        }
        stringifiedAttributes += '=' + attributes[attributeName];
    }
    try {
        var keyNamePair = encodeURIComponent(String(key)) + "=" + encodeURIComponent(String(value));
        return doc.cookie = keyNamePair + stringifiedAttributes;
    } catch (e) {
        return void 0;
    }
};
exports.setCookie = setCookie;
var getCookie = function getCookie(doc, key) {
    var cookies = [];
    try {
        cookies = doc.cookie ? doc.cookie.split('; ') : [];
    } catch (e) {}
    var rdecode = /(%[0-9A-Z]{2})+/g;
    var result;
    for (var i = 0; i < cookies.length; i++) {
        var parts = cookies[i].split('=');
        var cookie = parts.slice(1).join('=');
        try {
            var name = parts[0].replace(rdecode, decodeURIComponent);
            cookie.replace(rdecode, decodeURIComponent);
            if (key === name) {
                result = decodeURIComponent(cookie);
                break;
            }
        } catch (e) {
            return void 0;
        }
    }
    return result;
};
exports.getCookie = getCookie;
var deleteCookie = function deleteCookie(doc, key, params) {
    exports.setCookie(doc, key, '', __assign(__assign({}, params), { expires: new Date(0) }));
};
exports.deleteCookie = deleteCookie;

/***/ }),
/* 18 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getInternalClass = void 0;
var toString = {}.toString;
var typeReg = /\[object (\w+)\]/;
var getInternalClass = function getInternalClass(target) {
    var typeString = toString.call(target);
    if (!typeString) {
        return null;
    }
    var match = typeString.match(typeReg);
    if (!match) {
        return null;
    }
    var type = match[1];
    if (!type) {
        return null;
    }
    return type;
};
exports.getInternalClass = getInternalClass;

/***/ }),
/* 19 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.xhrRequest = void 0;
function xhrRequest(type, link, withCredentials, cb, eb) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        return cb && cb(xhr);
    };
    xhr.onerror = function () {
        return eb && eb(xhr);
    };
    try {
        xhr.open(type, link);
        xhr.withCredentials = withCredentials;
        xhr.send();
    } catch (e) {
        eb && eb(xhr);
    }
}
exports.xhrRequest = xhrRequest;

/***/ }),
/* 20 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestNetwork = void 0;
var indexOf_1 = __webpack_require__(21);
var config_1 = __webpack_require__(2);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var ADBLOCK_REACTION_TIME = 2000;
var ATTEMPTS_MAX_COUNT = 2;
var TestNetwork = function (_super) {
    __extends(TestNetwork, _super);
    function TestNetwork() {
        var _this = _super.call(this) || this;
        _this.links = config_1.config.detect.links;
        return _this;
    }
    TestNetwork.prototype.teardown = function () {
        this.links = [];
    };
    TestNetwork.prototype.light = function (blockedResource) {
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    TestNetwork.prototype.heavy = function (callback, blockedResource) {
        var _this = this;
        if (!this.links.length) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        this.getDocument(function (doc) {
            if (doc) {
                _this.loadNextLink(doc, callback);
            } else {
                callback({
                    blocker: blockers_1.BLOCKERS.NOT_BLOCKED
                });
            }
        });
    };
    TestNetwork.prototype.getDocument = function (callback) {
        return callback(document);
    };
    TestNetwork.prototype.testLink = function (link, doc, attempts, cb, eb) {
        cb();
    };
    TestNetwork.prototype.loadNextLink = function (doc, callback, index) {
        var _this = this;
        if (index === void 0) {
            index = 0;
        }
        var link = this.links[index];
        if (!link) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        this.testLink(link, doc, ATTEMPTS_MAX_COUNT, function () {
            _this.loadNextLink(doc, callback, index + 1);
        }, function (l, timeStart, xhr) {
            var timeEnd = Date.now();
            if (timeEnd - timeStart > ADBLOCK_REACTION_TIME) {
                _this.loadNextLink(doc, callback, index + 1);
            } else {
                callback({
                    blocker: blockers_1.BLOCKERS.UNKNOWN,
                    resource: {
                        type: blockers_1.BlockedResourceType.NETWORK,
                        index: indexOf_1.indexOf(_this.links, link),
                        data: {
                            status: xhr ? xhr.status : 0,
                            statusText: xhr ? xhr.statusText : '',
                            method: link.type,
                            url: link.src,
                            time: timeEnd - timeStart
                        }
                    }
                });
            }
        });
    };
    return TestNetwork;
}(testBase_1.TestBase);
exports.TestNetwork = TestNetwork;

/***/ }),
/* 21 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.indexOf = void 0;
var indexOf = function indexOf(array, searchElement, fromIndex, comparator) {
    if (fromIndex === void 0) {
        fromIndex = 0;
    }
    if (comparator === void 0) {
        comparator = defaultComparator;
    }
    for (var i = fromIndex; i < array.length; i++) {
        if (comparator(array[i], searchElement)) {
            return i;
        }
    }
    return -1;
};
exports.indexOf = indexOf;
var defaultComparator = function defaultComparator(current, searchElement) {
    return current === searchElement;
};

/***/ }),
/* 22 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.generateHexString = void 0;
var random_1 = __webpack_require__(23);
function generateHexString(length) {
    var x = '';
    for (var key = 0; key < length; key++) {
        x += (random_1.random() * 16 | 0).toString(16);
    }
    return x;
}
exports.generateHexString = generateHexString;

/***/ }),
/* 23 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.random = void 0;
var IS_BROKEN_MATH_RANDOM_1 = __webpack_require__(59);
var pseudoRandom_1 = __webpack_require__(60);
function native() {
    return Math.random();
}
exports.random = IS_BROKEN_MATH_RANDOM_1.IS_BROKEN_MATH_RANDOM ? pseudoRandom_1.pseudoRandom : native;

/***/ }),
/* 24 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.removeNodeFromParent = void 0;
function removeNodeFromParent(node) {
    if (!node) {
        return;
    }
    var parentElement = node.parentElement;
    if (parentElement) {
        parentElement.removeChild(node);
    }
}
exports.removeNodeFromParent = removeNodeFromParent;

/***/ }),
/* 25 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.decode = exports.encode = exports.getKey = void 0;
var base64 = __webpack_require__(14);
var config_1 = __webpack_require__(2);
var decodedKey = null;
function getKey() {
    if (decodedKey === null) {
        var key = base64.addEquals(config_1.config.encode.key);
        decodedKey = base64.decodeUInt8String(key);
    }
    return decodedKey;
}
exports.getKey = getKey;
function xor(data, key) {
    var result = [];
    for (var i = 0; i < data.length; i++) {
        var xored = data.charCodeAt(i) ^ key.charCodeAt(i % key.length);
        result.push(String.fromCharCode(xored));
    }
    return result.join('');
}
function encode(url, utf8) {
    if (utf8 === void 0) {
        utf8 = false;
    }
    var encodeFn = utf8 ? base64.encode : base64.encodeUInt8String;
    var xoredUrl = xor(url, getKey());
    return base64.cropEquals(encodeFn(xoredUrl));
}
exports.encode = encode;
function decode(encodedUrl, utf8, key) {
    var decodeFn = utf8 ? base64.decode : base64.decodeUInt8String;
    var fullUrl = base64.addEquals(encodedUrl);
    return xor(decodeFn(fullUrl), key || getKey());
}
exports.decode = decode;

/***/ }),
/* 26 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.get = void 0;
var isObject_1 = __webpack_require__(27);
var get = function get(obj, path) {
    for (var _i = 0, _a = path.split('.'); _i < _a.length; _i++) {
        var key = _a[_i];
        if (!isObject_1.isObject(obj)) {
            obj = void 0;
            break;
        }
        obj = obj[key];
    }
    return obj;
};
exports.get = get;

/***/ }),
/* 27 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

Object.defineProperty(exports, "__esModule", { value: true });
exports.isObject = void 0;
function isObject(value) {
    var type = typeof value === "undefined" ? "undefined" : _typeof(value);
    return Boolean(value) && (type === 'object' || type === 'function');
}
exports.isObject = isObject;

/***/ }),
/* 28 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.onlyDisplayNoneInStyles = exports.checkIfStyleBlocker = void 0;
var displayNoneRE = /{\s*display\s*:\s*none\s*!important;(\s*color:\s*#?[\w\(\),\s]+!important;\s*background-color:\s*#?[\w\(\),\s]+!important;)?(\s*background:\s*#?[\w\(\),\s]+;)?\s*}/gi;
function getCSSRules(style) {
    var result = [];
    try {
        result = style.sheet.cssRules;
    } catch (e) {}
    return result;
}
function checkIfStyleBlocker(styles, selectorsCount) {
    for (var i = 0; i < styles.length; i++) {
        var styleText = styles[i].innerHTML;
        if (!onlyDisplayNoneInStyles(styleText) || styleText === '') {
            continue;
        }
        if (selectorsCount === 0) {
            return true;
        }
        var rules = getCSSRules(styles[i]);
        if (checkCountOfSelectors(rules, selectorsCount)) {
            return true;
        }
    }
    return false;
}
exports.checkIfStyleBlocker = checkIfStyleBlocker;
function onlyDisplayNoneInStyles(styleText) {
    var cleared = styleText.replace(displayNoneRE, '');
    return cleared.indexOf('{') === -1;
}
exports.onlyDisplayNoneInStyles = onlyDisplayNoneInStyles;
function checkCountOfSelectors(rules, count) {
    if (!rules.length) {
        return false;
    }
    var selectorText = rules[0].selectorText;
    if (!selectorText) {
        return false;
    }
    var selectors = selectorText.split(',');
    return selectors.length === count;
}

/***/ }),
/* 29 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isInjectInFrames = void 0;
var checkNativeCode_1 = __webpack_require__(4);
function isInjectInFrames(signatureString, win) {
    if (win === void 0) {
        win = window;
    }
    var elements = [win.HTMLFrameElement, win.HTMLIFrameElement, win.HTMLObjectElement];
    for (var i = 0; i < elements.length; i++) {
        if (elements[i] && elements[i].prototype) {
            var contentWindow = Object.getOwnPropertyDescriptor(elements[i].prototype, 'contentWindow');
            if (contentWindow && contentWindow.get && !checkNativeCode_1.checkNativeCode(contentWindow.get) && signatureString && contentWindow.get.toString().indexOf(signatureString) !== -1) {
                return true;
            }
        }
    }
    return false;
}
exports.isInjectInFrames = isInjectInFrames;

/***/ }),
/* 30 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isFirefox = void 0;
var isFirefox = function isFirefox(win) {
    if (win === void 0) {
        win = window;
    }
    return (/firefox/.test(win.navigator.userAgent.toLowerCase())
    );
};
exports.isFirefox = isFirefox;

/***/ }),
/* 31 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getWindowLocation = void 0;
function getWindowLocation(window) {
    if (window && window.location) {
        var location = window.location;
        if (typeof location.toString === 'function') {
            return location.toString();
        } else {
            return location.href || '';
        }
    } else {
        return '';
    }
}
exports.getWindowLocation = getWindowLocation;

/***/ }),
/* 32 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isUCBrowser = void 0;
var isUCBrowser = function isUCBrowser(win) {
    if (win === void 0) {
        win = window;
    }
    return win.navigator.userAgent.indexOf('UCBrowser') > -1;
};
exports.isUCBrowser = isUCBrowser;

/***/ }),
/* 33 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIfIsAndroid = void 0;
var isWindowsPhone_1 = __webpack_require__(34);
function getIfIsAndroid(win) {
    if (win === void 0) {
        win = window;
    }
    var _a = win.navigator.userAgent,
        userAgent = _a === void 0 ? '' : _a;
    var hasAndroidString = /Android/.test(userAgent);
    var isWindowsPhone = isWindowsPhone_1.isWindowsPhone(win);
    var isAndroidAdsSDK = /com\.yandex\.mobile\.metrica\.ads\.sdk.*?Android/.test(userAgent);
    return hasAndroidString && !isWindowsPhone || isAndroidAdsSDK;
}
exports.getIfIsAndroid = getIfIsAndroid;

/***/ }),
/* 34 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isWindowsPhone = void 0;
var isWindowsPhone = function isWindowsPhone(win) {
    if (win === void 0) {
        win = window;
    }
    return (/Windows Phone/i.test(win.navigator.userAgent)
    );
};
exports.isWindowsPhone = isWindowsPhone;

/***/ }),
/* 35 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsYaBrowser = void 0;
var getUserAgent_1 = __webpack_require__(3);
var getIsYaBrowser = function getIsYaBrowser(win) {
    if (win === void 0) {
        win = window;
    }
    return (/YaBrowser/.test(getUserAgent_1.getUserAgent(win))
    );
};
exports.getIsYaBrowser = getIsYaBrowser;

/***/ }),
/* 36 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isOperaMini = void 0;
var isOperaMini = function isOperaMini(win) {
    if (win === void 0) {
        win = window;
    }
    var userAgent = win.navigator.userAgent;
    if (userAgent.indexOf('Opera Mini') !== -1) {
        return true;
    }
    if (userAgent.indexOf('; wv)') !== -1 && userAgent.indexOf(' OPR/') !== -1) {
        return true;
    }
    if (userAgent.indexOf(' OPiOS/') !== -1) {
        return true;
    }
    return false;
};
exports.isOperaMini = isOperaMini;

/***/ }),
/* 37 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isTouchDevice = void 0;
var hasDocumentTouch_1 = __webpack_require__(104);
var hasTouchEvents_1 = __webpack_require__(105);
var hasTouchPoints_1 = __webpack_require__(106);
var isMatchingAnyPointerCoarse_1 = __webpack_require__(108);
var isMatchingTouchEnabled_1 = __webpack_require__(109);
function isTouchDevice(win) {
    if (win === void 0) {
        win = window;
    }
    return hasTouchPoints_1.hasTouchPoints(win) || isMatchingAnyPointerCoarse_1.isMatchingAnyPointerCoarse(win) || isMatchingTouchEnabled_1.isMatchingTouchEnabled(win) || hasDocumentTouch_1.hasDocumentTouch(win) || hasTouchEvents_1.hasTouchEvents(win);
}
exports.isTouchDevice = isTouchDevice;

/***/ }),
/* 38 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isMatchingMediaQuery = void 0;
var isFunction_1 = __webpack_require__(8);
var isObject_1 = __webpack_require__(27);
function isMatchingMediaQuery(win, query) {
    if (!isFunction_1.isFunction(win.matchMedia)) {
        return false;
    }
    var mediaQueryList = win.matchMedia(query);
    return isObject_1.isObject(mediaQueryList) && Boolean(mediaQueryList.matches);
}
exports.isMatchingMediaQuery = isMatchingMediaQuery;

/***/ }),
/* 39 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.testProperty = void 0;
var prefixes_1 = __webpack_require__(16);
var testProperty = function testProperty(condition, pfxs) {
    if (pfxs === void 0) {
        pfxs = prefixes_1.cssPrefixes;
    }
    if (!condition) {
        return false;
    }
    var _a = condition.split(':'),
        property = _a[0],
        value = _a[1];
    if (!value) {
        value = 'none';
    }
    if (window.CSS && window.CSS.supports) {
        for (var i = 0; i < pfxs.length; i++) {
            if (window.CSS.supports(pfxs[i] + property, value)) {
                return true;
            }
        }
        return false;
    }
    var testElem = new Image();
    for (var i = 0; i < pfxs.length; i++) {
        testElem.style.cssText = pfxs[i] + property + ":" + value;
        if (testElem.style.length) {
            return true;
        }
    }
    return false;
};
exports.testProperty = testProperty;

/***/ }),
/* 40 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var inject = __webpack_require__(41);
module.exports = inject;

/***/ }),
/* 41 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
var cookie_1 = __webpack_require__(17);
var crc32_1 = __webpack_require__(43);
var detect_1 = __webpack_require__(44);
var elGamal_1 = __webpack_require__(81);
var logger_1 = __webpack_require__(84);
var signatureName = 'sign';
function checkSignature(sign) {
    var _a = sign.split(','),
        r = _a[0],
        s = _a[1];
    var url = normalizeUrl(window.location.href.replace(/sign=\d+%2C\d+$/gi, ''));
    if (url[url.length - 1] !== '&' && url[url.length - 1] !== '?') {
        url = url + (url.indexOf('?') === -1 ? '?' : '&');
    }
    return elGamal_1.checkSign(crc32_1.crc32(url), {
        p: 958159042169167,
        g: 574559613,
        y: 614458371438293
    }, {
        r: Number(r),
        s: Number(s)
    });
}
function normalizeUrl(url) {
    return encodeURI(decodeURI(url)).replace(/#[\w\W]+$/gi, '');
}
function buildUrl(location) {
    var prefix;
    switch (location.protocol) {
        case 'http:':
            prefix = 'h';
            break;
        case 'https:':
            prefix = 's';
            break;
        default:
            prefix = 'n';
    }
    return "https://yandex.ru/turbo/" + prefix + "/" + location.host + location.pathname + "?utm_source=aab&platform=desktop";
}
function redirect() {
    window.location.href = buildUrl(window.location);
}
function run() {
    var exceptions = ['/admin', '/login', '/wp-admin', '/wp-login', '/auth', '/sso', '/register'];
    var path = window.location.pathname;
    if (path) {
        for (var i = 0; i < exceptions.length; i++) {
            if (path.indexOf(exceptions[i]) === 0) {
                return;
            }
        }
    }
    var queryString = (window.location.search || '').substring(1);
    var queryParams = queryString.split('&');
    for (var i = 0; i < queryParams.length; i++) {
        var _a = queryParams[i].split('='),
            name = _a[0],
            value = _a[1];
        if (name === signatureName) {
            var expires = new Date();
            expires.setDate(expires.getDate() + 14);
            cookie_1.setCookie(document, signatureName, value, {
                path: window.location.pathname,
                expires: expires
            });
            break;
        }
    }
    var cookies = [];
    try {
        cookies = document.cookie ? document.cookie.split('; ') : [];
    } catch (e) {}
    for (var i = 0; i < cookies.length; i++) {
        var parts = cookies[i].split('=');
        var name = parts[0];
        var cookie = parts.slice(1).join('=');
        if (signatureName === name && cookie && checkSignature(decodeURIComponent(unescape(cookie)))) {
            return;
        }
    }
    detect_1.detect(function (result, blockedResource) {
        logger_1.log(result, blockedResource);
        if (result.blocked) {
            redirect();
        }
    });
}
try {
    run();
} catch (e) {
    redirect();
}

/***/ }),
/* 42 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isDate = void 0;
var getInternalClass_1 = __webpack_require__(18);
var isDate = function isDate(maybeDate) {
    return maybeDate instanceof Date || getInternalClass_1.getInternalClass(maybeDate) === 'Date';
};
exports.isDate = isDate;

/***/ }),
/* 43 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.crc32 = void 0;
var signedCRCTable = function () {
    var table = new Array(256);
    var c = 0;
    for (var i = 0; i < 256; i++) {
        c = i;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        c = c & 1 ? -306674912 ^ c >>> 1 : c >>> 1;
        table[i] = c;
    }
    return typeof Int32Array !== 'undefined' ? new Int32Array(table) : table;
}();
function crc32(string) {
    var result = -1;
    var i = 0;
    while (i < string.length) {
        var charCode = string.charCodeAt(i++);
        if (charCode < 0x80) {
            result = result >>> 8 ^ signedCRCTable[(result ^ charCode) & 0xff];
        } else if (charCode < 0x800) {
            result = result >>> 8 ^ signedCRCTable[(result ^ (192 | charCode >> 6 & 31)) & 0xff];
            result = result >>> 8 ^ signedCRCTable[(result ^ (128 | charCode & 63)) & 0xff];
        } else if (charCode >= 0xd800 && charCode < 0xe000) {
            charCode = (charCode & 1023) + 64;
            var d = string.charCodeAt(i++) & 1023;
            result = result >>> 8 ^ signedCRCTable[(result ^ (240 | charCode >> 8 & 7)) & 0xff];
            result = result >>> 8 ^ signedCRCTable[(result ^ (128 | charCode >> 2 & 63)) & 0xff];
            result = result >>> 8 ^ signedCRCTable[(result ^ (128 | d >> 6 & 15 | (charCode & 3) << 4)) & 0xff];
            result = result >>> 8 ^ signedCRCTable[(result ^ (128 | d & 63)) & 0xff];
        } else {
            result = result >>> 8 ^ signedCRCTable[(result ^ (224 | charCode >> 12 & 15)) & 0xff];
            result = result >>> 8 ^ signedCRCTable[(result ^ (128 | charCode >> 6 & 63)) & 0xff];
            result = result >>> 8 ^ signedCRCTable[(result ^ (128 | charCode & 63)) & 0xff];
        }
    }
    result = ~result;
    return result < 0 ? 0xffffffff + result + 1 : result;
}
exports.crc32 = crc32;

/***/ }),
/* 44 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

Object.defineProperty(exports, "__esModule", { value: true });
exports.detect = exports.addCallbackToQueue = exports.isDetected = exports.isRunning = exports.resetYaAdb = void 0;
var cookie_1 = __webpack_require__(17);
var config_1 = __webpack_require__(2);
var isUserCrawler_1 = __webpack_require__(47);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var testInstant_1 = __webpack_require__(48);
var testLayout_1 = __webpack_require__(49);
var TestXhr =  true ? __webpack_require__(52).TestXhr : testBase_1.TestBase;
var TestScript =  true ? __webpack_require__(53).TestScript : testBase_1.TestBase;
var TestIframes =  true ? __webpack_require__(55).TestIframes : testBase_1.TestBase;
var isByPass_1 = __webpack_require__(67);
var scrambler_1 = __webpack_require__(25);
var CONTEXT_KEY = scrambler_1.encode('aab_context');
var Ya = protectInit(window, 'Ya');
var YaAdb = protectInit(Ya, CONTEXT_KEY);
var OUTER_CALLBACKS_KEY = scrambler_1.encode('antiadb_callback');
var IS_DETECTED_KEY = scrambler_1.encode('antiadb_detected');
var IS_RUNNING_KEY = scrambler_1.encode('antiadb_running');
var CURRENT_BLOCKER_KEY = scrambler_1.encode('antiadb_blocker');
var BLOCK_REMOVE_COOKIE = 'blcrm';
if (!YaAdb[CURRENT_BLOCKER_KEY]) {
    YaAdb[CURRENT_BLOCKER_KEY] = scrambler_1.encode(blockers_1.BLOCKERS.NOT_BLOCKED);
}
if (!YaAdb[OUTER_CALLBACKS_KEY]) {
    YaAdb[OUTER_CALLBACKS_KEY] = [];
}
function protectInit(obj, property) {
    var defaultValue = { __proto__: null, hasOwnProperty: Object.hasOwnProperty };
    try {
        var result = obj[property] = obj[property] || defaultValue;
        if (!result || (typeof result === "undefined" ? "undefined" : _typeof(result)) !== 'object') {
            result = defaultValue;
        }
        return result;
    } catch (e) {}
    return defaultValue;
}
function ready(callback) {
    if (document.readyState === 'complete' || document.readyState === 'interactive') {
        callback();
    } else {
        document.addEventListener('DOMContentLoaded', function () {
            callback();
        });
    }
}
function resetYaAdb() {
    YaAdb[OUTER_CALLBACKS_KEY] = [];
    YaAdb[IS_RUNNING_KEY] = false;
    YaAdb[IS_DETECTED_KEY] = false;
    YaAdb[CURRENT_BLOCKER_KEY] = scrambler_1.encode(blockers_1.BLOCKERS.NOT_BLOCKED);
}
exports.resetYaAdb = resetYaAdb;
function isRunning() {
    return YaAdb && YaAdb[IS_RUNNING_KEY];
}
exports.isRunning = isRunning;
function isDetected() {
    return YaAdb && YaAdb[IS_DETECTED_KEY];
}
exports.isDetected = isDetected;
function addCallbackToQueue(callback) {
    YaAdb[OUTER_CALLBACKS_KEY].push(callback);
}
exports.addCallbackToQueue = addCallbackToQueue;
function run() {
    var isReadyDocument = document.readyState === 'complete' || document.readyState === 'interactive';
    var baseChecks = [testInstant_1.TestInstant, TestXhr];
    if (isReadyDocument) {
        baseChecks = [testInstant_1.TestInstant, testLayout_1.TestLayout, TestXhr, TestScript, TestIframes];
    }
    checkTests(baseChecks, function (baseResult) {
        if (baseResult.blocker === blockers_1.BLOCKERS.NOT_BLOCKED) {
            if (!isReadyDocument) {
                ready(function () {
                    var baseChecks = [testInstant_1.TestInstant, testLayout_1.TestLayout, TestScript, TestIframes];
                    checkTests(baseChecks, function (baseResult) {
                        detectBlockerType(baseResult);
                    });
                });
                return;
            }
        }
        detectBlockerType(baseResult);
    });
}
function detect(callback) {
    if (YaAdb[OUTER_CALLBACKS_KEY].indexOf(callback) !== -1) {
        return;
    }
    addCallbackToQueue(callback);
    var result = {
        blocker: blockers_1.BLOCKERS.NOT_BLOCKED,
        blocked: false
    };
    if (cookie_1.getCookie(document, BLOCK_REMOVE_COOKIE)) {
        finalize({
            blocker: blockers_1.BLOCKERS.ADGUARD,
            blocked: true,
            fakeDetect: true
        });
        return;
    }
    if (isUserCrawler_1.isUserCrawler() || isByPass_1.isByPass() || !isModernBrowser()) {
        finalize(result);
        return;
    }
    if (!config_1.config.force) {
        if (YaAdb[IS_DETECTED_KEY]) {
            var blocker = scrambler_1.decode(YaAdb[CURRENT_BLOCKER_KEY]);
            if (blocker !== blockers_1.BLOCKERS.NOT_BLOCKED) {
                result = {
                    blocker: blocker,
                    blocked: true,
                    fakeDetect: false
                };
            }
            finalize(result);
            return;
        }
        if (isRunning()) {
            return;
        }
    }
    YaAdb[IS_RUNNING_KEY] = true;
    run();
}
exports.detect = detect;
function protect(callback) {
    var result = {
        blocker: blockers_1.BLOCKERS.NOT_BLOCKED
    };
    try {
        return callback();
    } catch (e) {
        result = {
            blocker: blockers_1.BLOCKERS.UNKNOWN,
            resource: {
                type: blockers_1.BlockedResourceType.EXCEPTION,
                data: {
                    message: e && e.message
                }
            }
        };
    }
    return result;
}
function teardown(checks) {
    for (var i = 0; i < checks.length; i++) {
        checks[i].teardown();
    }
}
function detectBlockerType(baseResult) {
    if (baseResult.blocker !== blockers_1.BLOCKERS.UNKNOWN) {
        finalize({
            blocker: baseResult.blocker,
            blocked: baseResult.blocker !== blockers_1.BLOCKERS.NOT_BLOCKED
        }, baseResult.resource);
        return;
    }
    var typeChecks = [testBase_1.TestBase];
    if (true) {
        typeChecks = [__webpack_require__(68).TestGhostery, __webpack_require__(69).TestAdblock, __webpack_require__(70).TestUblock, __webpack_require__(71).TestAdguard, __webpack_require__(75).TestKIS, __webpack_require__(76).TestFFTrackingProtection, __webpack_require__(77).TestBrowser, __webpack_require__(79).TestDNS, __webpack_require__(80).TestUkraine];
    }
    checkTests(typeChecks, function (result) {
        finalize({
            blocker: result.blocker,
            blocked: result.blocker !== blockers_1.BLOCKERS.NOT_BLOCKED
        }, baseResult.resource);
    }, blockers_1.BLOCKERS.UNKNOWN, baseResult.resource);
}
function checkTests(tests, callback, defaultBlocker, blockedResource) {
    if (defaultBlocker === void 0) {
        defaultBlocker = blockers_1.BLOCKERS.NOT_BLOCKED;
    }
    var result = {
        blocker: defaultBlocker
    };
    var instances = [];
    var _loop_1 = function _loop_1(i) {
        var instant = new tests[i]();
        instances.push(instant);
        result = protect(function () {
            return instant.light(blockedResource);
        });
        if (result.blocker !== blockers_1.BLOCKERS.NOT_BLOCKED) {
            teardown(instances);
            callback(result);
            return { value: void 0 };
        }
    };
    for (var i = 0; i < tests.length; i++) {
        var state_1 = _loop_1(i);
        if ((typeof state_1 === "undefined" ? "undefined" : _typeof(state_1)) === "object") return state_1.value;
    }
    var testsComplete = 0;
    var triggered = false;
    var _loop_2 = function _loop_2(i) {
        protect(function () {
            return instances[i].heavy(function (heavyResult) {
                testsComplete++;
                instances[i].teardown();
                if (triggered) {
                    return;
                }
                if (heavyResult.blocker !== blockers_1.BLOCKERS.NOT_BLOCKED) {
                    triggered = true;
                    callback(heavyResult);
                    return;
                }
                var allTestCompleted = tests.length === testsComplete;
                if (allTestCompleted) {
                    triggered = true;
                    callback({
                        blocker: defaultBlocker
                    });
                }
            }, blockedResource);
        });
    };
    for (var i = 0; i < instances.length; i++) {
        _loop_2(i);
    }
}
function isModernBrowser() {
    return window.addEventListener && window.getComputedStyle && Function.prototype.bind;
}
function finalize(result, blockedResource) {
    YaAdb[CURRENT_BLOCKER_KEY] = scrambler_1.encode(result.blocker);
    YaAdb[IS_DETECTED_KEY] = true;
    for (var i = 0; i < YaAdb[OUTER_CALLBACKS_KEY].length; i++) {
        YaAdb[OUTER_CALLBACKS_KEY][i](result, blockedResource);
    }
    YaAdb[OUTER_CALLBACKS_KEY] = [];
    YaAdb[IS_RUNNING_KEY] = false;
}

/***/ }),
/* 45 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.AdblockCookieMatchingType = void 0;
var AdblockCookieMatchingType;
(function (AdblockCookieMatchingType) {
    AdblockCookieMatchingType[AdblockCookieMatchingType["doNotMatch"] = 0] = "doNotMatch";
    AdblockCookieMatchingType[AdblockCookieMatchingType["image"] = 1] = "image";
    AdblockCookieMatchingType[AdblockCookieMatchingType["refresh"] = 2] = "refresh";
    AdblockCookieMatchingType[AdblockCookieMatchingType["all"] = 3] = "all";
    AdblockCookieMatchingType[AdblockCookieMatchingType["scrumble"] = 4] = "scrumble";
})(AdblockCookieMatchingType = exports.AdblockCookieMatchingType || (exports.AdblockCookieMatchingType = {}));

/***/ }),
/* 46 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.M = void 0;
exports.M = '__ADB_CONFIG__';

/***/ }),
/* 47 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isUserCrawler = void 0;
var crawlersList =  false ? [] : [/1c\+enterprise\//i, /24bb\.bot/i, /360(spider|user-agent)/i, /\+xml/i, /^192\.comagent$/i, /^[0-9a-za-z._]*@/i, /^appie 1\.1 (www\.walhello\.com)$/i, /^asynchttpclient 1\.0$/i, /^bdfetch$/i, /^blogpulselive (support@blogpulse\.com)$/i, /^browsershots$/i, /^cinetic_htdig$/i, /^cjb\.net proxy$/i, /^csci_b659\/0\.13$/i, /^ejupiter\.com$/i, /^exactseek\.com$/i, /^holmes\/3\.9 (onet\.pl)$/i, /^internet explorer$/i, /^java\//i, /^linkdex\.com\/v2\.0$/i, /^livedoor screenshot\/0\.10$/i, /^ng\/2\.0$/i, /^noyona_0_1$/i, /^opencalaissemanticproxy$/i, /^pagepeeker\.com$/i, /^panscient\.com$/i, /^qseero/i, /^search\.kumkie\.com$/i, /^seocheck \(fischernetzdesign seo checker, info@fischernetzdesign\.de\)$/i, /^shelob \(shelob@gmx\.net\)$/i, /^silk\/1\.0$/i, /^surphace scout&v4\.0 - scout at surphace dot com$/i, /^virus_detector (virus_harvester@securecomputing\.com)$/i, /^woko 3\.0*/i, /^xml sitemaps generator/i, /^yaanb\/1\.5\.001 \(compatible; win64;\)$/i, /acoon/i, /ad municher/i, /adre/i, /afinethttp/i, /agent\//i, /aggreagtor/i, /agropoisk\//i, /al_viewer/i, /all\.by/i, /anonym(\s|$)/i, /anonynous/i, /apache/i, /api\//i, /aport/i, /appengine-google/i, /asdfghjkl/i, /ask jeeves\/teoma/i, /asterias\//i, /autoluba\//i, /backlink-check/i, /baiduspider/i, /bigli seo/i, /bingbot/i, /bitrix/i, /bits\//i, /blaiz-bee\//i, /bloglines/i, /boitho\.com-dc\//i, /bond\//i, /brytetoolbar/i, /btwwebclient/i, /ccubee/i, /check_http\//i, /checker/i, /chilkat\//i, /cjnetworkquality/i, /clever internet suite/i, /coccoc/i, /component/i, /coralwebprx\//i, /crawl\//i, /crowsnest/i, /curl/i, /dataparksearch/i, /digger\//i, /disco watchman/i, /download master/i, /download\//i, /downloader\//i, /drupal/i, /duckduckbot/i, /envolk\//i, /european search engine\//i, /everest-vulcan inc/i, /facebookexternalhit/i, /fast search/i, /feed demon/i, /feed/i, /feedburner/i, /feedfetcher-google/i, /feedsky crawler/i, /feedzirra/i, /fetch\//i, /filangy\//i, /fileboost\.net\//i, /filter\//i, /find\//i, /ftp\//i, /gethtml\//i, /getright\//i, /getrightpro\//i, /gibbon/i, /gobblegobble\//i, /google web preview/i, /googlebot/i, /grab\//i, /graber\//i, /hatena antenna\//i, /hatena/i, /hoowwwer\//i, /href-fetcher\//i, /http::lite\//i, /http_client/i, /httpclient\//i, /httpget/i, /httpsendrequestex/i, /httpsession/i, /httrack/i, /ia_archiver/i, /iaskspider/i, /ichiro\//i, /images\//i, /indy library/i, /ineturl:\//i, /info\.web/i, /ingrid\//i, /internetseer/i, /intravnews\//i, /ipcheck/i, /isa server connectivity check/i, /isilox\//i, /jetbrains/i, /js-kit/i, /kanban\//i, /keepaliveclient/i, /kretrieve\//i, /libfetch\//i, /libwww/i, /liferea\//i, /links/i, /loader\//i, /lth\//i, /ltx71/i, /luki/i, /lwp/i, /mail\.ru/i, /microsearch\.ru/i, /microsoft-cryptoapi/i, /mnogosearch/i, /mr http monitor/i, /ms web services client protocol/i, /msnbot/i, /najdi\.si\//i, /net snippets/i, /net::trackback\//i, /netcarta_webmapper\//i, /netcraft ssl server survey/i, /netcraftsurveyagent/i, /netintelligence liveassessment - www\.netintelligence\.com/i, /netmonitor\//i, /netpumper\//i, /newsblur/i, /newsgator/i, /NING/, /nutch/i, /okhttp\//i, /onlinewebcheck/i, /package http/i, /page_verifier/i, /pagepromoter/i, /parser/i, /partner, search yn\//i, /pastukhov/i, /perl/i, /[^e]php/i, /ping/i, /pogodak\.co\.yu/i, /portalmmm/i, /posh/i, /postrank/i, /puxarapido/i, /python/i, /queryseekerspider/i, /rambler/i, /reader\//i, /redir\//i, /restsharp/i, /ripper\//i, /robot|crawler|spider|[a-z]*bot(\s|$)/i, /robozilla/i, /rss grabber/i, /rss reader/i, /rss\//i, /ruby/i, /rv:x/i, /scanner\//i, /scanyandex/i, /scooter\//i, /script/i, /search\//i, /seek\//i, /semantic analyzer/i, /semonitor/i, /seonewstop\//i, /shadowwebanalyzer/i, /shopwiki\//i, /simplepie/i, /simplesubmit/i, /sitemap generator\//i, /slackbot-linkexpanding/i, /snappy\//i, /snarfer\//i, /snoopy/i, /soap/i, /sogou(\s|spider)/i, /soso[ a-z\-]*?spider/i, /speedy spider/i, /sproose\//i, /squidclam/i, /stackrambler/i, /steroid download/i, /susie/i, /szukacz\//i, /targetyournews/i, /tatpoisk/i, /teleport pro\//i, /teleport ultra\//i, /thumbnails\//i, /trendiction/i, /tron\/siteposition/i, /trustlink client/i, /turtle\//i, /twitterbot/i, /url/i, /url\//i, /useragent/i, /utilmind httpget/i, /validator\//i, /validurl/i, /vbseo/i, /verifier\//i, /vhod search/i, /w3c/i, /watznew agent/i, /webcopier/i, /wget/i, /wget/i, /winhttprequest/i, /wordpress/i, /www\.nsoftware\.com/i, /xenu link sleuth/i, /xfruits/i, /xianguo\.com/i, /y!j-bsc\//i, /y!j-psc\//i, /yahoo!\s+slurp|yahooseeker/i, /yahoo-blogs\//i, /yahoo-mmaudvid\//i, /yahoocachesystem/i, /yahoofeedseeker\//i, /yahooseeker-testing\//i, /yahoovideosearch/i, /yahooysmcm\//i, /yandesk/i, /yandexaddurl/i, /yandexantivirus/i, /yandexbot/i, /yandexbot/i, /yandexcatalog/i, /yandexdirect/i, /yandexfavicons/i, /yandeximageresizer/i, /yandeximages/i, /yandexmedia/i, /yandexmetrika/i, /yandexnews/i, /yandexvideo/i, /you?daobot/i, /zyborg\//i];
function getUserAgent() {
    return navigator.userAgent;
}
function isUserCrawler() {
    if (false) {
        return false;
    }
    var userAgent = getUserAgent();
    for (var i = 0; i < crawlersList.length; i++) {
        if (crawlersList[i].test(userAgent)) {
            return true;
        }
    }
    return false;
}
exports.isUserCrawler = isUserCrawler;

/***/ }),
/* 48 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestInstant = void 0;
var blockers_1 = __webpack_require__(0);
var getShadowRootText_1 = __webpack_require__(11);
var testBase_1 = __webpack_require__(1);
var RandomNameRE = /^[a-z][a-z0-9]{6}$/;
var TestInstant = function (_super) {
    __extends(TestInstant, _super);
    function TestInstant() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestInstant.prototype.light = function (blockedResource) {
        var result = this.testAbortOnPropertyRead();
        if (result) {
            return {
                blocker: blockers_1.BLOCKERS.UNKNOWN,
                resource: {
                    type: blockers_1.BlockedResourceType.INSTANT,
                    data: {
                        message: result
                    }
                }
            };
        }
        result = this.testShadow();
        if (result) {
            return {
                blocker: blockers_1.BLOCKERS.UNKNOWN,
                resource: {
                    type: blockers_1.BlockedResourceType.INSTANT,
                    data: {
                        message: result
                    }
                }
            };
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    TestInstant.prototype.testAbortOnPropertyRead = function () {
        var fields = ['Element.prototype.attachShadow'];
        try {
            for (var i = 0; i < fields.length; i++) {
                var path = fields[i].split('.');
                var current = window;
                for (var j = 0; j < path.length; j++) {
                    current = current[path[j]];
                }
            }
        } catch (e) {
            if (e instanceof ReferenceError && RandomNameRE.test(e.message)) {
                return String(e);
            }
        }
        return '';
    };
    TestInstant.prototype.testShadow = function () {
        var shadowRootText = getShadowRootText_1.getShadowRootText();
        if (shadowRootText.indexOf('get()') !== -1) {
            return shadowRootText;
        }
        try {
            if (Element.prototype.attachShadow && Element.prototype.attachShadow.toString().indexOf('MutationObserver') !== -1) {
                return Element.prototype.attachShadow.toString();
            }
        } catch (e) {
            return String(e);
        }
        return '';
    };
    return TestInstant;
}(testBase_1.TestBase);
exports.TestInstant = TestInstant;

/***/ }),
/* 49 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestLayout = void 0;
var filter_1 = __webpack_require__(50);
var reduce_1 = __webpack_require__(51);
var some_1 = __webpack_require__(12);
var isInIframe_1 = __webpack_require__(13);
var config_1 = __webpack_require__(2);
var blockers_1 = __webpack_require__(0);
var layoutTestElement_1 = __webpack_require__(6);
var testBase_1 = __webpack_require__(1);
var MAX_AWAIT_TIME = 3000;
var ADBLOCK_REACTION_TIME = 500;
var invisibleTags = ['SCRIPT', 'TITLE', 'META', 'HEAD', 'STYLE'];
var TestLayout = function (_super) {
    __extends(TestLayout, _super);
    function TestLayout() {
        var _this = _super.call(this) || this;
        _this.divs = [];
        _this.divs = _this.createDivs();
        return _this;
    }
    TestLayout.prototype.teardown = function () {
        layoutTestElement_1.removeElements(this.divs);
        this.divs = [];
    };
    TestLayout.prototype.light = function (blockedResource) {
        if (isInIframe_1.isInIframe(window)) {
            return {
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            };
        }
        var hiddenElement = this.checkHidden(this.divs);
        if (hiddenElement) {
            return {
                blocker: blockers_1.BLOCKERS.UNKNOWN,
                resource: {
                    type: blockers_1.BlockedResourceType.ELEMENT,
                    data: {
                        element: hiddenElement,
                        styles: this.getStylesObject(hiddenElement),
                        sheets: this.getStyleSheets(hiddenElement)
                    },
                    index: 0
                }
            };
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    TestLayout.prototype.heavy = function (callback, blockedResource) {
        if (isInIframe_1.isInIframe(window) || !this.divs.length) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        this.checkHiddenHeavy(this.divs, callback);
    };
    TestLayout.prototype.getChildren = function (element) {
        return filter_1.filter(Array.prototype.slice.call(element.children), function (child) {
            return child instanceof HTMLElement;
        });
    };
    TestLayout.prototype.createDivs = function () {
        var custom = config_1.config.detect.custom;
        var divs = [];
        for (var i = 0; i < custom.length; i++) {
            divs.push(layoutTestElement_1.createDivWithContent(custom[i]));
        }
        return divs;
    };
    TestLayout.prototype.checkHiddenHeavy = function (elements, callback, startTime) {
        var _this = this;
        if (startTime === void 0) {
            startTime = Number(new Date());
        }
        var hiddenElement = this.checkHidden(elements);
        if (hiddenElement) {
            callback({
                blocker: blockers_1.BLOCKERS.UNKNOWN,
                resource: {
                    type: blockers_1.BlockedResourceType.ELEMENT,
                    data: {
                        element: hiddenElement,
                        styles: this.getStylesObject(hiddenElement),
                        sheets: this.getStyleSheets(hiddenElement)
                    },
                    index: 0
                }
            });
        } else {
            var now = Number(new Date());
            if (now - startTime < MAX_AWAIT_TIME) {
                setTimeout(function () {
                    _this.checkHiddenHeavy(elements, callback, startTime);
                }, ADBLOCK_REACTION_TIME);
            } else {
                callback({
                    blocker: blockers_1.BLOCKERS.NOT_BLOCKED
                });
            }
        }
    };
    TestLayout.prototype.checkHidden = function (elements) {
        var _this = this;
        var hiddenElement = reduce_1.reduce(elements, function (acc, element, i) {
            if (!acc) {
                acc = _this.getHiddenElement(_this.getChildren(element)[0]);
            }
            return acc;
        }, void 0);
        return hiddenElement;
    };
    TestLayout.prototype.getHiddenElement = function (element) {
        var _this = this;
        if (invisibleTags.indexOf(element.tagName) !== -1) {
            return void 0;
        }
        var computed = window.getComputedStyle(element);
        var display = computed.display,
            left = computed.left,
            top = computed.top,
            opacity = computed.opacity,
            visibility = computed.visibility,
            transform = computed.transform;
        var styleLeft = element.style.left;
        var styleTop = element.style.top;
        var result;
        if (display === 'none' || opacity !== '1' || visibility !== 'visible' || left !== 'auto' && left !== '0px' && left !== styleLeft || top !== 'auto' && top !== '0px' && top !== styleTop || transform === 'matrix(0, 0, 0, 0, 0, 0)') {
            result = element;
        }
        result = result || reduce_1.reduce(this.getChildren(element), function (acc, child) {
            return acc || _this.getHiddenElement(child);
        }, void 0);
        return result;
    };
    TestLayout.prototype.getStylesObject = function (el) {
        var result = {};
        try {
            var styles = ['display', 'width', 'height', 'left', 'top', 'opacity', 'visibility', 'transform'];
            var computed = window.getComputedStyle(el);
            for (var i = 0; i < styles.length; i++) {
                var name = styles[i];
                result[name] = computed[name];
            }
        } catch (e) {}
        return result;
    };
    TestLayout.prototype.matches = function (element, selector) {
        var proto = Element.prototype;
        var matches = proto.matches || proto.matchesSelector || proto.webkitMatchesSelector || proto.mozMatchesSelector || proto.msMatchesSelector || proto.oMatchesSelector;
        if (!matches) {
            var elements = document.querySelectorAll(selector);
            return some_1.some(elements, function (e) {
                return e === element;
            });
        }
        return matches.call(element, selector);
    };
    TestLayout.prototype.getStyleSheets = function (el) {
        var result = [];
        try {
            var sheets = document.styleSheets;
            if (!sheets) {
                return [];
            }
            for (var i in sheets) {
                try {
                    var rules = sheets[i].rules || sheets[i].cssRules || [];
                    for (var r in rules) {
                        if (this.matches(el, rules[r].selectorText)) {
                            result.push({
                                node: sheets[i].ownerNode,
                                rule: rules[r].cssText
                            });
                            break;
                        }
                    }
                } catch (e) {}
            }
        } catch (e) {}
        return result;
    };
    return TestLayout;
}(testBase_1.TestBase);
exports.TestLayout = TestLayout;

/***/ }),
/* 50 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.filter = void 0;
var filter = function filter(array, callback, thisArg) {
    var result = [];
    for (var index = 0; index < array.length; index++) {
        var value = array[index];
        if (callback.call(thisArg, value, index, array)) {
            result.push(value);
        }
    }
    return result;
};
exports.filter = filter;

/***/ }),
/* 51 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.reduce = void 0;
var reduce = function reduce(array, callback, initialValue) {
    var index = 0;
    if (arguments.length < 3) {
        index = 1;
        initialValue = array[0];
    }
    for (; index < array.length; index++) {
        initialValue = callback(initialValue, array[index], index, array);
    }
    return initialValue;
};
exports.reduce = reduce;

/***/ }),
/* 52 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestXhr = void 0;
var xhrRequest_1 = __webpack_require__(19);
var testNetwork_1 = __webpack_require__(20);
var TestXhr = function (_super) {
    __extends(TestXhr, _super);
    function TestXhr() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestXhr.prototype.testLink = function (link, doc, attempts, cb, eb) {
        var _this = this;
        var errback = function errback(xhr) {
            if (attempts > 1) {
                return _this.testLink(link, doc, attempts - 1, cb, eb);
            }
            var status = xhr ? xhr.status : 0;
            var statusText = xhr ? xhr.statusText : '';
            eb(link, timeStart, {
                status: status,
                statusText: statusText
            });
        };
        var timeStart = Date.now();
        switch (link.type) {
            case 'head':
            case 'post':
            case 'get':
                xhrRequest_1.xhrRequest(link.type.toUpperCase(), link.src, false, cb, errback);
                break;
            default:
                cb();
        }
    };
    return TestXhr;
}(testNetwork_1.TestNetwork);
exports.TestXhr = TestXhr;

/***/ }),
/* 53 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestScript = void 0;
var scriptRequest_1 = __webpack_require__(54);
var layoutTestElement_1 = __webpack_require__(6);
var testNetwork_1 = __webpack_require__(20);
var TestScript = function (_super) {
    __extends(TestScript, _super);
    function TestScript() {
        var _this = _super.call(this) || this;
        _this.iframe = layoutTestElement_1.createTestIframe(1, 1);
        return _this;
    }
    TestScript.prototype.teardown = function () {
        _super.prototype.teardown.call(this);
        layoutTestElement_1.removeElement(this.iframe);
    };
    TestScript.prototype.getDocument = function (callback) {
        var iframe = this.iframe;
        if (!iframe) {
            callback();
            return void 0;
        }
        var iframeDocument = iframe.contentDocument;
        if (!iframeDocument && iframe.contentWindow) {
            iframeDocument = iframe.contentWindow.document;
        }
        if (iframeDocument && (iframeDocument.readyState === 'complete' || iframeDocument.readyState === 'interactive')) {
            iframeDocument.write = function () {
                'use strict';
            };
            callback(iframeDocument);
        } else {
            iframe.onload = function () {
                iframe.onload = null;
                iframeDocument = iframe.contentDocument;
                if (!iframeDocument && iframe.contentWindow) {
                    iframeDocument = iframe.contentWindow.document;
                }
                iframeDocument.write = function () {
                    'use strict';
                };
                callback(iframeDocument);
            };
        }
        return iframeDocument;
    };
    TestScript.prototype.testLink = function (link, doc, attempts, cb, eb) {
        var _this = this;
        var errback = function errback(xhr) {
            if (attempts > 1) {
                return _this.testLink(link, doc, attempts - 1, cb, eb);
            }
            var status = xhr ? xhr.status : 0;
            var statusText = xhr ? xhr.statusText : '';
            eb(link, timeStart, {
                status: status,
                statusText: statusText
            });
        };
        var timeStart = Date.now();
        switch (link.type) {
            case 'img':
            case 'script':
                scriptRequest_1.scriptRequest(link.type, link.src, doc, cb, errback);
                break;
            default:
                cb();
        }
    };
    return TestScript;
}(testNetwork_1.TestNetwork);
exports.TestScript = TestScript;

/***/ }),
/* 54 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.scriptRequest = void 0;
function scriptRequest(type, link, document, cb, eb) {
    var element = document.createElement(type);
    element.onload = function () {
        return cb && cb();
    };
    element.onerror = function () {
        return eb && eb();
    };
    element.src = link;
    document.body.appendChild(element);
}
exports.scriptRequest = scriptRequest;

/***/ }),
/* 55 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestIframes = void 0;
var every_1 = __webpack_require__(56);
var find_1 = __webpack_require__(57);
var forEach_1 = __webpack_require__(58);
var indexOf_1 = __webpack_require__(21);
var map_1 = __webpack_require__(7);
var generateHexString_1 = __webpack_require__(22);
var isInIframe_1 = __webpack_require__(13);
var getNativeJSON_1 = __webpack_require__(63);
var base64_1 = __webpack_require__(14);
var config_1 = __webpack_require__(2);
var blockers_1 = __webpack_require__(0);
var layoutTestElement_1 = __webpack_require__(6);
var testBase_1 = __webpack_require__(1);
var JSON = getNativeJSON_1.getNativeJSON(window);
var MAX_TIME_AWAIT = 5000;
var EVENT_MESSAGE = 'message';
var TestIframes = function (_super) {
    __extends(TestIframes, _super);
    function TestIframes() {
        var _this = _super.call(this) || this;
        _this.iframes = config_1.config.detect.iframes;
        return _this;
    }
    TestIframes.prototype.heavy = function (callback, blockedResource) {
        var _this = this;
        if (!this.iframes.length) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        if (isInIframe_1.isInIframe(window)) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        var framesInfo = map_1.map(this.iframes, function (url) {
            var sign = generateHexString_1.generateHexString(8);
            return {
                element: _this.createIframe(url, sign, config_1.config.pid),
                isHandled: false,
                url: url,
                sign: sign
            };
        });
        var findFrameInfo = function findFrameInfo(origin) {
            return find_1.find(framesInfo, function (info) {
                return info.url.indexOf(origin) === 0;
            });
        };
        var getIsFinished = function getIsFinished() {
            return every_1.every(framesInfo, function (info) {
                return info.isHandled;
            });
        };
        var removeFrames = function removeFrames() {
            forEach_1.forEach(framesInfo, function (info) {
                return layoutTestElement_1.removeElement(info.element);
            });
        };
        var timeout;
        var handler = function handler(event) {
            var frameInfo = findFrameInfo(event.origin);
            if (!frameInfo) {
                return;
            }
            var frameResult;
            try {
                frameResult = event.data && JSON.parse(event.data) || null;
            } catch (e) {
                frameResult = null;
            }
            if (!frameResult || frameResult.sign !== frameInfo.sign) {
                return;
            }
            frameInfo.isHandled = true;
            if (frameResult.blocked || getIsFinished()) {
                window.removeEventListener(EVENT_MESSAGE, handler);
                removeFrames();
                clearTimeout(timeout);
                if (frameResult.blocked) {
                    callback({
                        blocker: frameResult.blocker,
                        resource: {
                            type: blockers_1.BlockedResourceType.IN_IFRAME,
                            data: {
                                url: frameInfo.element.src
                            },
                            index: indexOf_1.indexOf(_this.iframes, frameInfo, 0, function (url, info) {
                                return url.indexOf(info.url) === 0;
                            })
                        }
                    });
                } else {
                    callback({
                        blocker: blockers_1.BLOCKERS.NOT_BLOCKED
                    });
                }
            }
        };
        window.addEventListener(EVENT_MESSAGE, handler);
        timeout = window.setTimeout(function () {
            window.removeEventListener(EVENT_MESSAGE, handler);
            removeFrames();
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
        }, MAX_TIME_AWAIT);
    };
    TestIframes.prototype.createIframe = function (url, sign, pid) {
        var parameters = base64_1.encode("pid=" + pid + "&sign=" + sign);
        var src = url + "#" + parameters;
        return layoutTestElement_1.createTestIframe(100, 100, src);
    };
    return TestIframes;
}(testBase_1.TestBase);
exports.TestIframes = TestIframes;

/***/ }),
/* 56 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.every = void 0;
var every = function every(array, callback) {
    for (var index = 0; index < array.length; index++) {
        if (!callback(array[index], index, array)) {
            return false;
        }
    }
    return true;
};
exports.every = every;

/***/ }),
/* 57 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.find = void 0;
var checkNativeCode_1 = __webpack_require__(4);
var ponyfill = function ponyfill(array, predicate) {
    for (var i = 0; i < array.length; i++) {
        var value = array[i];
        if (predicate(value, i, array)) {
            return value;
        }
    }
    return void 0;
};
var nativeMethod = [].find;
var nativeFind = function nativeFind(array, predicate) {
    return nativeMethod.call(array, predicate);
};
exports.find = checkNativeCode_1.checkNativeCode(nativeMethod) ? nativeFind : ponyfill;

/***/ }),
/* 58 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.forEach = void 0;
var forEach = function forEach(array, callback, thisArg) {
    for (var index = 0; index < array.length; index++) {
        callback.call(thisArg, array[index], index, array);
    }
};
exports.forEach = forEach;

/***/ }),
/* 59 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.IS_BROKEN_MATH_RANDOM = void 0;
var checkNativeCode_1 = __webpack_require__(4);
exports.IS_BROKEN_MATH_RANDOM = !checkNativeCode_1.checkNativeCode(Math.random) || Math.random() === Math.random();

/***/ }),
/* 60 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.pseudoRandom = void 0;
var performanceNow_1 = __webpack_require__(61);
var MODULUS = 2147483647;
var MULTIPLIER = 16807;
var seed = Date.now() * performanceNow_1.performanceNow() % MODULUS;
function next() {
    seed = seed * MULTIPLIER % MODULUS;
    return seed;
}
var DENOMINATOR = MODULUS - 1;
function pseudoRandom() {
    return (next() - 1) / DENOMINATOR;
}
exports.pseudoRandom = pseudoRandom;

/***/ }),
/* 61 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.performanceNow = exports.performanceNowShim = exports.isPerformanceNowSupported = void 0;
var dateNow_1 = __webpack_require__(62);
var isFunction_1 = __webpack_require__(8);
var performance = typeof window === 'undefined' ? void 0 : window.performance;
exports.isPerformanceNowSupported = performance && isFunction_1.isFunction(performance.now);
var nowOffset = performance && performance.timing && performance.timing.navigationStart ? performance.timing.navigationStart : dateNow_1.dateNow();
var lastPerfNow = 0;
var performanceNowShim = function performanceNowShim() {
    var perfNow = dateNow_1.dateNow() - nowOffset;
    lastPerfNow = Math.max(perfNow, lastPerfNow);
    return lastPerfNow;
};
exports.performanceNowShim = performanceNowShim;
exports.performanceNow = exports.isPerformanceNowSupported ? function () {
    return performance.now();
} : function () {
    return exports.performanceNowShim();
};

/***/ }),
/* 62 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.dateNow = void 0;
var isFunction_1 = __webpack_require__(8);
var isSupported = Date && isFunction_1.isFunction(Date.now);
exports.dateNow = isSupported ? function () {
    return Date.now();
} : function () {
    return new Date().getTime();
};

/***/ }),
/* 63 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.checkNativeJSON = exports.getNativeJSON = exports.extractJSONFromIframe = void 0;
var createHiddenFriendlyIFrame_1 = __webpack_require__(64);
var removeIframe_1 = __webpack_require__(66);
var checkNativeCode_1 = __webpack_require__(4);
var nativeJSON;
var extractJSONFromIframe = function extractJSONFromIframe(parent) {
    if (parent === void 0) {
        parent = document.body;
    }
    var iframe = createHiddenFriendlyIFrame_1.createHiddenFriendlyIFrame(parent);
    var iframeJSON = iframe.contentWindow.JSON;
    return {
        JSON: iframeJSON,
        clean: function clean() {
            return removeIframe_1.removeIframe(iframe);
        }
    };
};
exports.extractJSONFromIframe = extractJSONFromIframe;
function getNativeJSON(win) {
    if (win === void 0) {
        win = window;
    }
    if (nativeJSON === void 0) {
        if (checkNativeJSON(win)) {
            nativeJSON = win.JSON;
        } else {
            nativeJSON = {
                stringify: executeNativeJSONMethod('stringify'),
                parse: executeNativeJSONMethod('parse')
            };
        }
    }
    return nativeJSON;
}
exports.getNativeJSON = getNativeJSON;
var executeNativeJSONMethod = function executeNativeJSONMethod(method) {
    return function (value) {
        var _a = exports.extractJSONFromIframe(),
            JSON = _a.JSON,
            clean = _a.clean;
        try {
            return JSON[method](value);
        } finally {
            clean();
        }
    };
};
function checkNativeJSON(win) {
    if (win === void 0) {
        win = window;
    }
    return win.JSON && checkNativeCode_1.checkNativeCode(win.JSON.stringify) && checkNativeCode_1.checkNativeCode(win.JSON.parse);
}
exports.checkNativeJSON = checkNativeJSON;

/***/ }),
/* 64 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.createHiddenFriendlyIFrame = void 0;
var createFriendlyIFrame_1 = __webpack_require__(65);
function createHiddenFriendlyIFrame(parentElement) {
    var iframe = createFriendlyIFrame_1.createFriendlyIFrame(parentElement);
    iframe.width = '0';
    iframe.height = '0';
    iframe.style.position = 'absolute';
    return iframe;
}
exports.createHiddenFriendlyIFrame = createHiddenFriendlyIFrame;

/***/ }),
/* 65 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.createFriendlyIFrame = void 0;
function createFriendlyIFrame(parentElement, _a) {
    var _b = _a === void 0 ? {} : _a,
        content = _b.content,
        size = _b.size;
    var iframe = parentElement.ownerDocument.createElement('iframe');
    iframe.scrolling = 'no';
    iframe.setAttribute('allowfullscreen', '');
    iframe.style.display = 'block';
    if (size) {
        iframe.height = size.height;
        iframe.width = size.width;
    }
    parentElement.appendChild(iframe);
    var contentDocument = iframe.contentDocument;
    contentDocument.open();
    if (content) {
        contentDocument.write(content);
    }
    contentDocument.close();
    contentDocument.body.style.margin = '0';
    iframe.style.borderWidth = '0';
    return iframe;
}
exports.createFriendlyIFrame = createFriendlyIFrame;

/***/ }),
/* 66 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.removeIframe = void 0;
var removeNodeFromParent_1 = __webpack_require__(24);
function removeIframe(iframe) {
    iframe.src = '';
    removeNodeFromParent_1.removeNodeFromParent(iframe);
}
exports.removeIframe = removeIframe;

/***/ }),
/* 67 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isByPass = void 0;
var adbconfig = __webpack_require__(2);
function isByPass() {
    if (!adbconfig.config.dbltsr) {
        return false;
    }
    var diff = Math.abs(Number(new Date(adbconfig.config.dbltsr)) - Number(new Date()));
    return diff < 1000 * 60 * 60 * 2;
}
exports.isByPass = isByPass;

/***/ }),
/* 68 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestGhostery = void 0;
var some_1 = __webpack_require__(12);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var GHOSTERY = 'ghostery-';
var suffixes = ['purple-box', 'ghostery-box', 'ghostery-count', 'ghostery-title'];
var TestGhostery = function (_super) {
    __extends(TestGhostery, _super);
    function TestGhostery() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestGhostery.prototype.light = function (blockedResource) {
        if (this.testSelectors()) {
            return {
                blocker: blockers_1.BLOCKERS.GHOSTERY
            };
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    TestGhostery.prototype.heavy = function (callback, blockedResource) {
        var _this = this;
        setTimeout(function () {
            if (_this.testSelectors()) {
                callback({
                    blocker: blockers_1.BLOCKERS.GHOSTERY
                });
            } else {
                callback({
                    blocker: blockers_1.BLOCKERS.NOT_BLOCKED
                });
            }
        }, 750);
    };
    TestGhostery.prototype.testSelectors = function () {
        return some_1.some(suffixes, function (suffix) {
            var element = document.getElementById(GHOSTERY + suffix);
            return Boolean(element);
        });
    };
    return TestGhostery;
}(testBase_1.TestBase);
exports.TestGhostery = TestGhostery;

/***/ }),
/* 69 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestAdblock = void 0;
var getIsSafari_1 = __webpack_require__(9);
var get_1 = __webpack_require__(26);
var removeNodeFromParent_1 = __webpack_require__(24);
var blockers_1 = __webpack_require__(0);
var checkIfStyleBlocker_1 = __webpack_require__(28);
var getShadowRootText_1 = __webpack_require__(11);
var isInjectInFrames_1 = __webpack_require__(29);
var layoutTestElement_1 = __webpack_require__(6);
var testBase_1 = __webpack_require__(1);
var isOldOpera = function isOldOpera() {
    return window.opera && window.opera.version() < 13;
};
var TestAdblock = function (_super) {
    __extends(TestAdblock, _super);
    function TestAdblock() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestAdblock.prototype.light = function (blockedResource) {
        if (getIsSafari_1.getIsSafari() && this.testWebSocket() || this.testRTCPeerConnectionForFirefox() || isInjectInFrames_1.isInjectInFrames('injectIntoContentWindow') || this.testShadow() || this.testOldOpera()) {
            return {
                blocker: blockers_1.BLOCKERS.ADBLOCK
            };
        } else {
            return {
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            };
        }
    };
    TestAdblock.prototype.heavy = function (callback, blockedResource) {
        var iframe = layoutTestElement_1.createTestIframe(100, 100);
        setTimeout(function () {
            if (isInjectInFrames_1.isInjectInFrames('injectIntoContentWindow', iframe.contentWindow)) {
                callback({
                    blocker: blockers_1.BLOCKERS.ADBLOCK
                });
            } else {
                callback({
                    blocker: blockers_1.BLOCKERS.NOT_BLOCKED
                });
            }
            removeNodeFromParent_1.removeNodeFromParent(iframe);
        }, 1000);
    };
    TestAdblock.prototype.testWebSocket = function () {
        var WSConstructor = get_1.get(window, 'WebSocket.prototype.constructor');
        return WSConstructor && WSConstructor.toString().indexOf('WrappedWebSocket') !== -1;
    };
    TestAdblock.prototype.testRTCPeerConnectionForFirefox = function () {
        var setConfigurationFunction = get_1.get(window, 'RTCPeerConnection.prototype.setConfiguration');
        if (setConfigurationFunction && setConfigurationFunction.toString && setConfigurationFunction.toString().indexOf('checkRequest') !== -1 || window.RTCPeerConnection && RTCPeerConnection.toSource && RTCPeerConnection.toSource().indexOf('WrappedRTCPeerConnection') !== -1) {
            return true;
        }
        return false;
    };
    TestAdblock.prototype.testShadow = function () {
        var shadowRootText = getShadowRootText_1.getShadowRootText();
        if (shadowRootText.indexOf('get()') !== -1) {
            return true;
        }
        if (Element.prototype.attachShadow && Element.prototype.attachShadow.toString().indexOf('MutationObserver') !== -1) {
            return true;
        }
        return false;
    };
    TestAdblock.prototype.testOldOpera = function () {
        if (!isOldOpera()) {
            return false;
        }
        var styles = document.querySelectorAll('style');
        var isAdblockPlus = checkIfStyleBlocker_1.checkIfStyleBlocker(styles, 20);
        var isAdblock = checkIfStyleBlocker_1.checkIfStyleBlocker(styles, 0);
        return isAdblock || isAdblockPlus;
    };
    return TestAdblock;
}(testBase_1.TestBase);
exports.TestAdblock = TestAdblock;

/***/ }),
/* 70 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestUblock = void 0;
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var TestUblock = function (_super) {
    __extends(TestUblock, _super);
    function TestUblock() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestUblock.prototype.light = function (blockedResource) {
        var headStyles = document.querySelectorAll('head>style[type="text/css"]');
        for (var i = 0; i < headStyles.length; i++) {
            var innerHTML = headStyles[i].innerHTML;
            if (innerHTML.indexOf(':root') > -1 && innerHTML.indexOf('display: none !important;') > -1) {
                return {
                    blocker: blockers_1.BLOCKERS.UBLOCK
                };
            }
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    return TestUblock;
}(testBase_1.TestBase);
exports.TestUblock = TestUblock;

/***/ }),
/* 71 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestAdguard = void 0;
var getRandomString_1 = __webpack_require__(72);
var blockers_1 = __webpack_require__(0);
var checkIfStyleBlocker_1 = __webpack_require__(28);
var getShadowRootText_1 = __webpack_require__(11);
var isInjectInFrames_1 = __webpack_require__(29);
var testBase_1 = __webpack_require__(1);
var ADGUARD_UNIQ_RULES = ['ins[data-revive-zoneid]'];
var TestAdguard = function (_super) {
    __extends(TestAdguard, _super);
    function TestAdguard() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestAdguard.prototype.light = function (blockedResource) {
        if (this.isShadowRoot() || this.isModernAdguard() || this.isOldAdguard() || this.isStyleLink() || this.isAdguardInCSS() || this.isAdguardInCSSMobile() || isInjectInFrames_1.isInjectInFrames('injectPageScriptAPIInWindow')) {
            return {
                blocker: blockers_1.BLOCKERS.ADGUARD
            };
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    TestAdguard.prototype.heavy = function (callback, blockedResource) {
        if (!window.postMessage || !window.addEventListener) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        var timeoutId;
        var handler = function handler(event) {
            if (event.source === window && event.data.direction && event.data.direction === 'to-page-script@adguard') {
                removeHandler();
                clearTimeout(timeoutId);
                callback({
                    blocker: blockers_1.BLOCKERS.ADGUARD
                });
            }
        };
        var removeHandler = function removeHandler() {
            return window.removeEventListener('message', handler, false);
        };
        window.addEventListener('message', handler, false);
        timeoutId = window.setTimeout(function () {
            removeHandler();
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
        }, 500);
        window.postMessage({
            direction: 'from-page-script@adguard',
            elementUrl: getRandomString_1.getRandomString(10),
            documentUrl: document.location.href,
            block: getRandomString_1.getRandomString(10),
            requestType: 'DOCUMENT',
            requestId: getRandomString_1.getRandomString(10)
        }, '*');
    };
    TestAdguard.prototype.isShadowRoot = function () {
        var rootText = getShadowRootText_1.getShadowRootText();
        return rootText.indexOf('function () {') !== -1;
    };
    TestAdguard.prototype.isModernAdguard = function () {
        return window.AG_onLoad !== void 0;
    };
    TestAdguard.prototype.isOldAdguard = function () {
        return window.adguard !== void 0;
    };
    TestAdguard.prototype.isStyleLink = function () {
        var styleLinks = document.querySelectorAll('head>link[rel="stylesheet"]');
        for (var i = 0; i < styleLinks.length; i++) {
            var link = styleLinks[i];
            if (link.href.indexOf('adguard') > -1) {
                return true;
            }
        }
        return false;
    };
    TestAdguard.prototype.isAdguardInCSS = function () {
        var selectorsCount = 50;
        var shadowRoot = document.documentElement.shadowRoot;
        if (shadowRoot) {
            var styles = shadowRoot.querySelectorAll('style');
            var isAdguardInShadow = checkIfStyleBlocker_1.checkIfStyleBlocker(styles, selectorsCount);
            if (isAdguardInShadow) {
                return true;
            }
        }
        return checkIfStyleBlocker_1.checkIfStyleBlocker(document.head.querySelectorAll('style'), selectorsCount);
    };
    TestAdguard.prototype.isAdguardInCSSMobile = function () {
        var sheets = document.styleSheets || [];
        for (var i in sheets) {
            try {
                var rules = sheets[i].rules || sheets[i].cssRules || [];
                for (var r in rules) {
                    var rule = rules[r];
                    if (rule.style.length === 1 && rule.style.display === 'none' && rule.selectorText.length > 1000) {
                        for (var j in ADGUARD_UNIQ_RULES) {
                            if (rule.selectorText.indexOf(ADGUARD_UNIQ_RULES[j]) !== -1) {
                                return true;
                            }
                        }
                    }
                }
            } catch (e) {}
        }
        return false;
    };
    return TestAdguard;
}(testBase_1.TestBase);
exports.TestAdguard = TestAdguard;

/***/ }),
/* 72 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getRandomString = void 0;
var getRandomChar_1 = __webpack_require__(73);
function getRandomString(len) {
    var res = [];
    for (var i = 0; i < len; i++) {
        res.push(getRandomChar_1.getRandomChar());
    }
    return res.join('');
}
exports.getRandomString = getRandomString;

/***/ }),
/* 73 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getRandomChar = void 0;
var getRandomInt_1 = __webpack_require__(74);
function getRandomChar(from, to) {
    if (from === void 0) {
        from = 'a';
    }
    if (to === void 0) {
        to = 'z';
    }
    var charCode = getRandomInt_1.getRandomInt(from.charCodeAt(0), to.charCodeAt(0));
    return String.fromCharCode(charCode);
}
exports.getRandomChar = getRandomChar;

/***/ }),
/* 74 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getRandomInt = void 0;
var random_1 = __webpack_require__(23);
function getRandomInt(min, max) {
    var rand = min + random_1.random() * (max + 1 - min);
    rand = Math.floor(rand);
    return rand;
}
exports.getRandomInt = getRandomInt;

/***/ }),
/* 75 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestKIS = void 0;
var get_1 = __webpack_require__(26);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var ATTACH_SHADOW_SOURCE = ['Eleme', 'nt.pro', 'totype.atta', 'chShadow'].join('');
var toString = Function.prototype.toString;
var TestKIS = function (_super) {
    __extends(TestKIS, _super);
    function TestKIS() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestKIS.prototype.light = function (blockedResource) {
        try {
            var elem = get_1.get(window, ATTACH_SHADOW_SOURCE);
            if (elem) {
                var str = toString.call(elem);
                if (str && str.indexOf('KasperskyLab') !== -1) {
                    return {
                        blocker: blockers_1.BLOCKERS.KIS
                    };
                }
            }
        } catch (e) {}
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    return TestKIS;
}(testBase_1.TestBase);
exports.TestKIS = TestKIS;

/***/ }),
/* 76 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestFFTrackingProtection = void 0;
var isFirefox_1 = __webpack_require__(30);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var TestFFTrackingProtection = function (_super) {
    __extends(TestFFTrackingProtection, _super);
    function TestFFTrackingProtection() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestFFTrackingProtection.prototype.light = function (blockedResource) {
        if (isFirefox_1.isFirefox() && window.ImageBitmap && !navigator.serviceWorker) {
            return {
                blocker: blockers_1.BLOCKERS.FF_PRIVATE
            };
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    return TestFFTrackingProtection;
}(testBase_1.TestBase);
exports.TestFFTrackingProtection = TestFFTrackingProtection;

/***/ }),
/* 77 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestBrowser = void 0;
var isMobile_1 = __webpack_require__(78);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var TestBrowser = function (_super) {
    __extends(TestBrowser, _super);
    function TestBrowser() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestBrowser.prototype.light = function (blockedResource) {
        if (!isMobile_1.isMobile) {
            return {
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            };
        }
        var nav = navigator;
        if (nav.brave && nav.brave.isBrave) {
            return {
                blocker: blockers_1.BLOCKERS.BRAVE
            };
        }
        var ucapi = window.ucapi;
        if (ucapi && ucapi.hasOwnProperty('debug')) {
            return {
                blocker: blockers_1.BLOCKERS.UCBROWSER
            };
        }
        var ethereum = window.ethereum;
        if (ethereum && ethereum.providerName === 'opera') {
            return {
                blocker: blockers_1.BLOCKERS.OPERA_BROWSER
            };
        }
        return {
            blocker: blockers_1.BLOCKERS.NOT_BLOCKED
        };
    };
    return TestBrowser;
}(testBase_1.TestBase);
exports.TestBrowser = TestBrowser;

/***/ }),
/* 78 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isMobile = void 0;
var isMobile_1 = __webpack_require__(15);
exports.isMobile = isMobile_1.isMobile();

/***/ }),
/* 79 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestDNS = void 0;
var isMobile_1 = __webpack_require__(15);
var xhrRequest_1 = __webpack_require__(19);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var ADFOX_LINK = ['https://yas', 'tatic.net/pco', 'de/adfox/loa', 'der.js'].join('');
var CONTEXT_LINK = ['https://an.ya', 'ndex.ru/sy', 'stem/cont', 'ext.js'].join('');
var TestDNS = function (_super) {
    __extends(TestDNS, _super);
    function TestDNS() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestDNS.prototype.heavy = function (callback, blockedResource) {
        var _this = this;
        if (!isMobile_1.isMobile || !blockedResource || blockedResource.type !== blockers_1.BlockedResourceType.NETWORK) {
            callback({
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            });
            return;
        }
        this.request(ADFOX_LINK, function (adfoxResult) {
            if (!adfoxResult) {
                callback({
                    blocker: blockers_1.BLOCKERS.NOT_BLOCKED
                });
                return;
            }
            _this.request(CONTEXT_LINK, function (contextResult) {
                callback({
                    blocker: contextResult ? blockers_1.BLOCKERS.NOT_BLOCKED : blockers_1.BLOCKERS.DNS
                });
            });
        });
    };
    TestDNS.prototype.request = function (link, cb) {
        xhrRequest_1.xhrRequest('GET', link, false, function () {
            cb(true);
        }, function () {
            cb(false);
        });
    };
    return TestDNS;
}(testBase_1.TestBase);
exports.TestDNS = TestDNS;

/***/ }),
/* 80 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var __extends = void 0 && (void 0).__extends || function () {
    var _extendStatics = function extendStatics(d, b) {
        _extendStatics = Object.setPrototypeOf || { __proto__: [] } instanceof Array && function (d, b) {
            d.__proto__ = b;
        } || function (d, b) {
            for (var p in b) {
                if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p];
            }
        };
        return _extendStatics(d, b);
    };
    return function (d, b) {
        if (typeof b !== "function" && b !== null) throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
        _extendStatics(d, b);
        function __() {
            this.constructor = d;
        }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
}();
Object.defineProperty(exports, "__esModule", { value: true });
exports.TestUkraine = void 0;
var some_1 = __webpack_require__(12);
var blockers_1 = __webpack_require__(0);
var testBase_1 = __webpack_require__(1);
var TestUkraine = function (_super) {
    __extends(TestUkraine, _super);
    function TestUkraine() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    TestUkraine.prototype.light = function (blockedResource) {
        if (this.isUkrainianLocale()) {
            return {
                blocker: blockers_1.BLOCKERS.UK
            };
        } else {
            return {
                blocker: blockers_1.BLOCKERS.NOT_BLOCKED
            };
        }
    };
    TestUkraine.prototype.isUkrainianLocale = function () {
        var nav = navigator;
        var langs = [nav.language, nav.browserLanguage, nav.systemLanguage, nav.userLanguage];
        return some_1.some(langs, function (lang) {
            return Boolean(lang) && lang.toLowerCase() === 'uk';
        });
    };
    return TestUkraine;
}(testBase_1.TestBase);
exports.TestUkraine = TestUkraine;

/***/ }),
/* 81 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.checkSign = void 0;
var bigInt_1 = __webpack_require__(82);
function powmod(base, exp, modulus) {
    var accum = 1;
    var basepow2 = base;
    var arrP = bigInt_1.smallToArray(modulus);
    while (exp > 0) {
        if ((exp & 1) === 1) {
            accum = bigInt_1.multiplyWithBase(bigInt_1.smallToArray(accum), bigInt_1.smallToArray(basepow2), arrP);
            exp = exp - 1;
        }
        var arrBase = bigInt_1.smallToArray(basepow2);
        basepow2 = bigInt_1.multiplyWithBase(arrBase, arrBase, arrP);
        exp = exp / 2;
    }
    return accum;
}
function checkSign(hash, key, sign) {
    if (sign.r <= 0 || sign.r >= key.p || sign.s <= 0 || sign.s >= key.p - 1) {
        return false;
    }
    var y = powmod(key.y, sign.r, key.p);
    var r = powmod(sign.r, sign.s, key.p);
    var g = powmod(key.g, hash, key.p);
    var mul = bigInt_1.multiplyWithBase(bigInt_1.smallToArray(y), bigInt_1.smallToArray(r), bigInt_1.smallToArray(key.p));
    return mul === g;
}
exports.checkSign = checkSign;

/***/ }),
/* 82 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.multiplyWithBase = exports.arrayToSmall = exports.smallToArray = void 0;
var safeMath_1 = __webpack_require__(83);
var BASE = 1e7;
function smallToArray(n) {
    if (n < BASE) {
        return [n];
    }
    if (n < BASE * BASE) {
        return [n % BASE, safeMath_1.floor(n / BASE)];
    }
    return [n % BASE, safeMath_1.floor(n / BASE) % BASE, safeMath_1.floor(n / (BASE * BASE))];
}
exports.smallToArray = smallToArray;
function arrayToSmall(arr) {
    trim(arr);
    switch (arr.length) {
        case 0:
            return 0;
        case 1:
            return arr[0];
        case 2:
            return arr[0] + arr[1] * BASE;
        default:
            return arr[0] + (arr[1] + arr[2] * BASE) * BASE;
    }
}
exports.arrayToSmall = arrayToSmall;
function createArray(length) {
    var res = [];
    for (var i = 0; i < length; i++) {
        res[i] = 0;
    }
    return res;
}
function trim(v) {
    var i = v.length;
    while (v[--i] === 0) {}
    v.length = i + 1;
}
function compareAbs(a, b) {
    if (a.length !== b.length) {
        return a.length > b.length ? 1 : -1;
    }
    for (var i = a.length - 1; i >= 0; i--) {
        if (a[i] !== b[i]) {
            return a[i] > b[i] ? 1 : -1;
        }
    }
    return 0;
}
function subtract(a, b) {
    var r = createArray(a.length);
    var borrow = 0;
    var i;
    var difference;
    for (i = 0; i < b.length; i++) {
        difference = a[i] - borrow - b[i];
        if (difference < 0) {
            difference += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        r[i] = difference;
    }
    for (i = b.length; i < a.length; i++) {
        difference = a[i] - borrow;
        if (difference < 0) {
            difference += BASE;
        } else {
            r[i++] = difference;
            break;
        }
        r[i] = difference;
    }
    for (; i < a.length; i++) {
        r[i] = a[i];
    }
    trim(r);
    return r;
}
function mod(a, b) {
    var a_l = a.length;
    var result = [];
    var part = [];
    var guess;
    var xlen;
    var highx;
    var highy;
    var check;
    while (a_l) {
        part.unshift(a[--a_l]);
        trim(part);
        if (compareAbs(part, b) < 0) {
            result.push(0);
            continue;
        }
        xlen = part.length;
        highx = part[xlen - 1] * BASE + part[xlen - 2];
        highy = b[b.length - 1] * BASE + b[b.length - 2];
        if (xlen > b.length) {
            highx = (highx + 1) * BASE;
        }
        guess = safeMath_1.ceil(highx / highy);
        do {
            check = multiplyLong(b, smallToArray(guess));
            if (compareAbs(check, part) <= 0) {
                break;
            }
            guess--;
        } while (guess);
        result.push(guess);
        part = subtract(part, check);
    }
    result.reverse();
    return arrayToSmall(part);
}
function multiplyLong(a, b) {
    var len = a.length + b.length;
    var res = createArray(len);
    var product;
    var carry;
    var i;
    var a_i;
    var b_j;
    for (i = 0; i < a.length; ++i) {
        a_i = a[i];
        for (var j = 0; j < b.length; ++j) {
            b_j = b[j];
            product = a_i * b_j + res[i + j];
            carry = safeMath_1.floor(product / BASE);
            res[i + j] = product - carry * BASE;
            res[i + j + 1] += carry;
        }
    }
    trim(res);
    return res;
}
function multiplyWithBase(a, b, base) {
    return mod(multiplyLong(a, b), base);
}
exports.multiplyWithBase = multiplyWithBase;

/***/ }),
/* 83 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.ceil = exports.floor = void 0;
function floor(n) {
    return ~~n;
}
exports.floor = floor;
function ceil(n) {
    var fl = floor(n);
    return fl === n ? n : fl + 1;
}
exports.ceil = ceil;

/***/ }),
/* 84 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.log = exports.getDecodeLogNew = void 0;
var map_1 = __webpack_require__(7);
var getUserAgent_1 = __webpack_require__(3);
var getWindowLocation_1 = __webpack_require__(31);
var isInIframe_1 = __webpack_require__(13);
var assignProperties_1 = __webpack_require__(85);
var numbers_1 = __webpack_require__(87);
var safeLocalStorage = __webpack_require__(88);
var base64 = __webpack_require__(14);
var config_1 = __webpack_require__(2);
var blockers_1 = __webpack_require__(0);
var scrambler_1 = __webpack_require__(25);
var LOCAL_STORAGE_KEY = 'ludca';
var LOCAL_STORAGE_DELIMITER = 'bHVkY2E';
var OLD_LOCAL_STORAGE_DELIMITER = 'bHVkY2E=';
var VERSION_DELIMITER = 'dmVyc2lvbg';
var VERSION = scrambler_1.encode('2.0');
var encodeLog = function encodeLog(str) {
    return VERSION + VERSION_DELIMITER + base64.cropEquals(config_1.config.encode.key) + LOCAL_STORAGE_DELIMITER + scrambler_1.encode(str, true);
};
function parseLog(ludca) {
    var splittedLog = ludca.split('\n');
    if (splittedLog.length >= 4) {
        return {
            date: splittedLog[0],
            blockerType: splittedLog[1],
            blockedResource: splittedLog[2],
            userAgent: splittedLog[3],
            locationUrl: splittedLog[4]
        };
    }
    var oldFormatRE = /(.+?GMT)\s(\S+)\s(.*)/gi;
    var splittedOldLog = oldFormatRE.exec(ludca);
    if (splittedOldLog) {
        return {
            date: splittedOldLog[1],
            blockerType: void 0,
            blockedResource: splittedOldLog[2],
            userAgent: void 0,
            locationUrl: void 0
        };
    }
    return void 0;
}
function getDecodeLogNew() {
    var ludca = safeLocalStorage.getItem(LOCAL_STORAGE_KEY);
    if (ludca) {
        var parts = ludca.split(VERSION_DELIMITER);
        var keyAndData = ludca;
        var isOldVersion = true;
        var delimiter = OLD_LOCAL_STORAGE_DELIMITER;
        if (parts.length !== 1) {
            isOldVersion = false;
            keyAndData = parts[1];
            delimiter = LOCAL_STORAGE_DELIMITER;
        }
        var _a = keyAndData.split(delimiter),
            key = _a[0],
            data = _a[1];
        var txtLog = scrambler_1.decode(data.replace(/\+/g, '-').replace(/\//g, '_'), !isOldVersion, base64.decodeUInt8String(base64.addEquals(key)));
        return parseLog(txtLog);
    }
    return void 0;
}
exports.getDecodeLogNew = getDecodeLogNew;
function log(blockerResult, blockedResource) {
    if (blockerResult.blocker !== blockers_1.BLOCKERS.NOT_BLOCKED || blockerResult.fakeDetect) {
        if (blockerResult.fakeDetect && !blockedResource) {
            blockedResource = {
                type: blockers_1.BlockedResourceType.FAKE
            };
        }
        try {
            _log(blockerResult.blocker, blockedResource || {
                type: blockers_1.BlockedResourceType.UNKNOWN
            });
        } catch (e) {}
    }
}
exports.log = log;
function getConnectionType() {
    var result = '';
    var nav = navigator;
    try {
        var connection = nav.connection || nav.mozConnection || nav.webkitConnection;
        result = connection.type;
    } catch (e) {}
    return result;
}
function _log(blockerType, blockedResource) {
    var blockedResourceString = '';
    var additional = '';
    var code = '';
    var styles = '';
    var index = '';
    switch (blockedResource.type) {
        case blockers_1.BlockedResourceType.ELEMENT:
            var layout = blockedResource.data;
            blockedResourceString = "element";
            index = String(blockedResource.index);
            if (layout.element) {
                blockedResourceString += " " + layout.element.outerHTML;
                additional = map_1.map(layout.sheets || [], function (el) {
                    return "Rule: " + (el.rule || '') + "\nStyles: " + el.node.outerHTML;
                }).join('\n');
                if (layout.styles) {
                    styles = buildStylesString(layout.styles);
                }
            }
            code = 'e';
            break;
        case blockers_1.BlockedResourceType.NETWORK:
            var network = blockedResource.data;
            index = String(blockedResource.index);
            blockedResourceString = "network resource (" + network.method + ") " + network.url;
            additional = {
                connection: getConnectionType(),
                online: navigator.onLine
            };
            assignProperties_1.assignProperties(additional, network);
            code = 'l';
            break;
        case blockers_1.BlockedResourceType.IN_IFRAME:
            var iframe = blockedResource.data;
            index = String(blockedResource.index);
            blockedResourceString = "iframe " + iframe.url;
            code = 'f';
            break;
        case blockers_1.BlockedResourceType.FAKE:
            blockedResourceString = "fake detect ADGUARD";
            code = 'a';
            break;
        case blockers_1.BlockedResourceType.EXCEPTION:
            var exception = blockedResource.data;
            blockedResourceString = "exception " + exception.message;
            code = 'b';
            break;
        case blockers_1.BlockedResourceType.INSTANT:
            var instant = blockedResource.data;
            blockedResourceString = "instant check";
            additional = instant;
            code = 'b';
            break;
        default:
            blockedResourceString = 'unknown';
            code = 'u';
    }
    if (config_1.config.debug) {
        console.log("DETECTED " + blockerType + " BY " + blockedResourceString);
    }
    if (blockedResource.type !== blockers_1.BlockedResourceType.FAKE) {
        var dateString = new Date().toUTCString();
        var userAgent = getUserAgent_1.getUserAgent();
        var locationUrl = getWindowLocation_1.getWindowLocation(window);
        var encodedLog = encodeLog([dateString, blockerType, blockedResourceString, userAgent, locationUrl].join('\n'));
        safeLocalStorage.setItem(LOCAL_STORAGE_KEY, encodedLog);
    }
    var shouldLog = numbers_1.isPercent(config_1.config.additionalParams.detectLogPortion || 5);
    if (("boolean" !== 'boolean' || !false) && shouldLog) {
        __webpack_require__(90).sendNewDetectLog(blockerType, code + index, {
            element: blockedResourceString,
            additional: additional,
            styles: styles,
            inframe: isInIframe_1.isInIframe(window)
        });
    }
}
function buildStylesString(computed) {
    var styles = ['display', 'width', 'height', 'left', 'top', 'opacity', 'visibility', 'transform'];
    var result = [];
    for (var i = 0; i < styles.length; i++) {
        result.push(computed[styles[i]] || '');
    }
    return result.join(', ');
}

/***/ }),
/* 85 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.assignProperties = void 0;
var hasOwnProperty_1 = __webpack_require__(86);
function assignProperties(obj, props) {
    for (var key in props) {
        if (hasOwnProperty_1.hasOwnProperty(props, key)) {
            obj[key] = props[key];
        }
    }
}
exports.assignProperties = assignProperties;

/***/ }),
/* 86 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.hasOwnProperty = void 0;
function hasOwnProperty(obj, key) {
    return Object.prototype.hasOwnProperty.call(obj, key);
}
exports.hasOwnProperty = hasOwnProperty;

/***/ }),
/* 87 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isPercent = exports.fixPrecision = exports.toFraction = void 0;
var DIGITS = 3;
function toFraction(percent) {
    return fixPrecision(percent / 100);
}
exports.toFraction = toFraction;
function fixPrecision(number) {
    return Number(number.toFixed(DIGITS));
}
exports.fixPrecision = fixPrecision;
function isPercent(percent) {
    return Math.random() < toFraction(percent);
}
exports.isPercent = isPercent;

/***/ }),
/* 88 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.key = exports.clear = exports.removeItem = exports.getItem = exports.setItem = void 0;
var tryCatch_1 = __webpack_require__(89);
var setItem = function setItem(keyName, value) {
    return tryCatch_1.tryCatch(function () {
        return localStorage.setItem(keyName, value);
    });
};
exports.setItem = setItem;
var getItem = function getItem(keyName) {
    return tryCatch_1.tryCatch(function () {
        return localStorage.getItem(keyName);
    });
};
exports.getItem = getItem;
var removeItem = function removeItem(keyName) {
    return tryCatch_1.tryCatch(function () {
        return localStorage.removeItem(keyName);
    });
};
exports.removeItem = removeItem;
var clear = function clear() {
    return tryCatch_1.tryCatch(function () {
        return localStorage.clear();
    });
};
exports.clear = clear;
var key = function key(index) {
    return tryCatch_1.tryCatch(function () {
        return localStorage.key(index);
    });
};
exports.key = key;

/***/ }),
/* 89 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.tryCatch = void 0;
function tryCatch(fn, onError) {
    try {
        return fn();
    } catch (e) {
        if (typeof onError === 'function') {
            onError(e);
        }
    }
}
exports.tryCatch = tryCatch;

/***/ }),
/* 90 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.sendNewDetectLog = void 0;
var browser_1 = __webpack_require__(91);
var getWindowLocation_1 = __webpack_require__(31);
var generateHexString_1 = __webpack_require__(22);
var isNativeSendBeaconSupported_1 = __webpack_require__(118);
var config_1 = __webpack_require__(2);
var browser = browser_1.getBrowserName();
var device = browser_1.getDeviceName();
var sid = generateHexString_1.generateHexString(16);
var service = 'aab_detect';
var version = 'v0.2';
var NEW_LOG_URL = ['https://stat', 'ic-mon.yan', 'dex.net/adve', 'rt'].join('');
function sendNewDetectLog(blockerType, element, data) {
    var _a;
    var requestData = JSON.stringify({
        sid: sid,
        data: data,
        labels: {
            blocker: blockerType,
            element: element,
            pid: config_1.config.pid,
            browser: browser,
            device: device,
            version: version
        },
        tags: (_a = {}, _a["event_detect_" + blockerType] = 1, _a),
        location: getWindowLocation_1.getWindowLocation(window),
        timestamp: Date.now(),
        service: service,
        eventName: "detect_" + blockerType,
        eventType: 'event',
        value: 1,
        version: version
    });
    var requestUrl = NEW_LOG_URL + "?rnd=" + Math.ceil(Math.random() * 100);
    if (isNativeSendBeaconSupported_1.isNativeSendBeaconSupported(window)) {
        navigator.sendBeacon(requestUrl, requestData);
    } else {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', requestUrl, true);
        xhr.send(requestData);
    }
}
exports.sendNewDetectLog = sendNewDetectLog;

/***/ }),
/* 91 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.support = exports.getDeviceName = exports.getBrowserName = exports.testProperty = exports.flashVer = exports.isHDPIScreen = exports.isPhone = exports.isMobile = exports.isWindowsPhone = exports.androidVersion = exports.isAndroid = exports.iOSVersion = exports.isIOS = exports.isUCBrowser = exports.isYaBrowser = exports.isChrome = exports.isFirefox = exports.isSafariBasedBrowser = exports.safariVersion = exports.isSafari = exports.isEdge = exports.isIEQuirks = exports.isIE = exports.isIE11 = exports.isIE10 = exports.ieVersion = exports.isOperaMini = exports.isOpera = exports.isQuirks = void 0;
var getAndroidVersion_1 = __webpack_require__(92);
var getInternetExplorerVersion_1 = __webpack_require__(5);
var getIOSVersion_1 = __webpack_require__(94);
var isAndroid_1 = __webpack_require__(33);
var getIsChrome_1 = __webpack_require__(95);
var isFirefox_1 = __webpack_require__(30);
var isHDPIScreen_1 = __webpack_require__(96);
var getIsIOS_1 = __webpack_require__(10);
var isMobile_1 = __webpack_require__(15);
var isOpera_1 = __webpack_require__(99);
var isOperaMini_1 = __webpack_require__(36);
var isPhone_1 = __webpack_require__(100);
var getIsSafari_1 = __webpack_require__(9);
var getIsSafariBasedBrowser_1 = __webpack_require__(102);
var isTouchDevice_1 = __webpack_require__(37);
var isUCBrowser_1 = __webpack_require__(32);
var isWindowsPhone_1 = __webpack_require__(34);
var getIsYaBrowser_1 = __webpack_require__(35);
var getSafariVersion_1 = __webpack_require__(110);
var cssPropertySupport_1 = __webpack_require__(112);
var getIsLongUrlSupported_1 = __webpack_require__(113);
var getIsPostMessageSupported_1 = __webpack_require__(115);
var getIsQuirksMode_1 = __webpack_require__(116);
var win = window;
exports.isQuirks = getIsQuirksMode_1.getIsQuirks(win);
exports.isOpera = isOpera_1.isOpera(win);
exports.isOperaMini = isOperaMini_1.isOperaMini(win);
exports.ieVersion = getInternetExplorerVersion_1.getInternetExplorerVersion(win);
exports.isIE10 = exports.ieVersion === 10;
exports.isIE11 = exports.ieVersion === 11;
exports.isIE = exports.ieVersion > 0;
exports.isIEQuirks = getIsQuirksMode_1.getIsIEQuirks(win);
exports.isEdge = exports.ieVersion && exports.ieVersion > 11 || false;
exports.isSafari = getIsSafari_1.getIsSafari(win);
exports.safariVersion = getSafariVersion_1.getSafariVersion(win);
exports.isSafariBasedBrowser = getIsSafariBasedBrowser_1.getIsSafariBasedBrowser(win);
exports.isFirefox = isFirefox_1.isFirefox(win);
exports.isChrome = getIsChrome_1.getIsChrome(win);
exports.isYaBrowser = getIsYaBrowser_1.getIsYaBrowser(win);
exports.isUCBrowser = isUCBrowser_1.isUCBrowser(win);
exports.isIOS = getIsIOS_1.getIsIOS(win);
exports.iOSVersion = getIOSVersion_1.getIOSVersion(win);
exports.isAndroid = isAndroid_1.getIfIsAndroid(win);
exports.androidVersion = getAndroidVersion_1.getAndroidVersion(win);
exports.isWindowsPhone = isWindowsPhone_1.isWindowsPhone(win);
exports.isMobile = isMobile_1.isMobile(win);
exports.isPhone = isPhone_1.isPhone(win);
exports.isHDPIScreen = isHDPIScreen_1.isHDPIScreen(win);
exports.flashVer = [0, 0, 0];
var testProperty_1 = __webpack_require__(39);
Object.defineProperty(exports, "testProperty", { enumerable: true, get: function get() {
        return testProperty_1.testProperty;
    } });
var getBrowserName_1 = __webpack_require__(117);
Object.defineProperty(exports, "getBrowserName", { enumerable: true, get: function get() {
        return getBrowserName_1.getBrowserName;
    } });
function getDeviceName() {
    return exports.isPhone && 'phone' || exports.isMobile && 'tablet' || 'desktop';
}
exports.getDeviceName = getDeviceName;
exports.support = {
    cssFlex: cssPropertySupport_1.isCssFlexSupported,
    cssTransform: cssPropertySupport_1.isCssTransformSupported,
    cssTransition: cssPropertySupport_1.isCssTransitionSupported,
    cssFilterBlur: cssPropertySupport_1.isCssFilterBlurSupported,
    touch: isTouchDevice_1.isTouchDevice(win),
    postMessage: getIsPostMessageSupported_1.getIsPostMessageSupported(win),
    longUrls: getIsLongUrlSupported_1.getIsLongUrlSupported(win)
};

/***/ }),
/* 92 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getAndroidVersion = void 0;
function getAndroidVersion(win) {
    if (win === void 0) {
        win = window;
    }
    var ua = win.navigator.userAgent;
    var index = ua.indexOf('Android');
    if (index === -1) {
        return -1;
    } else {
        return parseFloat(ua.slice(index + 8));
    }
}
exports.getAndroidVersion = getAndroidVersion;

/***/ }),
/* 93 */
/***/ (function(module, exports) {

/**
 * detect IE
 * returns version of IE or false, if browser is not Internet Explorer
 */
var detectie = function(win) {
    if (!win) {
        win = window;
    }
    var ua = win.navigator.userAgent;

    var msie = ua.indexOf('MSIE ');
    if (msie > 0) {
        // IE 10 or older => return version number
        return parseInt(ua.substring(msie + 5, ua.indexOf('.', msie)), 10);
    }

    var trident = ua.indexOf('Trident/');
    if (trident > 0) {
        // IE 11 => return version number
        var rv = ua.indexOf('rv:');
        return parseInt(ua.substring(rv + 3, ua.indexOf('.', rv)), 10);
    }

    var edge = ua.indexOf('Edge/');
    if (edge > 0) {
       // IE 12 => return version number
       return parseInt(ua.substring(edge + 5, ua.indexOf('.', edge)), 10);
    }
    // other browser
    return false;
}

module.exports = detectie;


/***/ }),
/* 94 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIOSVersion = void 0;
var getIsIOS_1 = __webpack_require__(10);
function getIOSVersion(win) {
    if (win === void 0) {
        win = window;
    }
    if (getIsIOS_1.getIsIOS(win) && win.navigator) {
        var platform = win.navigator.platform;
        if (platform && /iP(hone|od|ad)/.test(platform)) {
            var version = win.navigator.appVersion.match(/OS (\d+)_(\d+)_?(\d+)?/);
            if (version) {
                return parseInt(version[1], 10);
            }
        }
    }
    return -1;
}
exports.getIOSVersion = getIOSVersion;

/***/ }),
/* 95 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsChrome = void 0;
var getIsYaBrowser_1 = __webpack_require__(35);
var getIsChrome = function getIsChrome(win) {
    if (win === void 0) {
        win = window;
    }
    return (/Chrome/.test(win.navigator.userAgent) && /Google Inc/.test(win.navigator.vendor) && !getIsYaBrowser_1.getIsYaBrowser(win)
    );
};
exports.getIsChrome = getIsChrome;

/***/ }),
/* 96 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isHDPIScreen = void 0;
var pixelRatio_1 = __webpack_require__(97);
var isHDPIScreen = function isHDPIScreen(win) {
    if (win === void 0) {
        win = window;
    }
    return pixelRatio_1.getPixelRatio(win) > 1;
};
exports.isHDPIScreen = isHDPIScreen;

/***/ }),
/* 97 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getPixelRatio = void 0;
var getPixelRatio_1 = __webpack_require__(98);
Object.defineProperty(exports, "getPixelRatio", { enumerable: true, get: function get() {
    return getPixelRatio_1.getPixelRatio;
  } });

/***/ }),
/* 98 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getPixelRatio = void 0;
var getPixelRatio = function getPixelRatio(win) {
    if (win === void 0) {
        win = window;
    }
    return win.devicePixelRatio || win.screen.deviceXDPI && win.screen.deviceXDPI / win.screen.logicalXDPI || 1;
};
exports.getPixelRatio = getPixelRatio;

/***/ }),
/* 99 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isOpera = void 0;
var isOpera = function isOpera(win) {
    if (win === void 0) {
        win = window;
    }
    return win.navigator.userAgent.indexOf('Opera') > -1 || win.navigator.userAgent.indexOf('OPR') > -1;
};
exports.isOpera = isOpera;

/***/ }),
/* 100 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isPhone = void 0;
var getUserAgent_1 = __webpack_require__(3);
var isAndroid_1 = __webpack_require__(33);
var getIsIphone_1 = __webpack_require__(101);
var isPhone = function isPhone(win) {
    if (win === void 0) {
        win = window;
    }
    return getIsIphone_1.getIsIphone(win) || isAndroid_1.getIfIsAndroid(win) && getUserAgent_1.getUserAgent(win).toLowerCase().indexOf('mobile') > -1;
};
exports.isPhone = isPhone;

/***/ }),
/* 101 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsIphone = void 0;
var getUserAgent_1 = __webpack_require__(3);
var getIsIOS_1 = __webpack_require__(10);
var getIsIphone = function getIsIphone(win) {
    if (win === void 0) {
        win = window;
    }
    return getIsIOS_1.getIsIOS(win) && getUserAgent_1.getUserAgent(win).toLowerCase().indexOf('iphone') > -1;
};
exports.getIsIphone = getIsIphone;

/***/ }),
/* 102 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsSafariBasedBrowser = void 0;
var getIsAppleTouchDevice_1 = __webpack_require__(103);
var getIsIOS_1 = __webpack_require__(10);
var getIsSafari_1 = __webpack_require__(9);
function getIsSafariBasedBrowser(win) {
    if (win === void 0) {
        win = window;
    }
    return getIsSafari_1.getIsSafari(win) || getIsIOS_1.getIsIOS(win) || getIsAppleTouchDevice_1.getIsAppleTouchDevice(win);
}
exports.getIsSafariBasedBrowser = getIsSafariBasedBrowser;

/***/ }),
/* 103 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsAppleTouchDevice = void 0;
var isTouchDevice_1 = __webpack_require__(37);
function getIsAppleTouchDevice(win) {
    if (win === void 0) {
        win = window;
    }
    return (/Apple/.test(win.navigator.vendor) && isTouchDevice_1.isTouchDevice(win)
    );
}
exports.getIsAppleTouchDevice = getIsAppleTouchDevice;

/***/ }),
/* 104 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.hasDocumentTouch = void 0;
function hasDocumentTouch(win) {
    var DocumentTouch = win.DocumentTouch;
    return Boolean(DocumentTouch) && win.document instanceof DocumentTouch;
}
exports.hasDocumentTouch = hasDocumentTouch;

/***/ }),
/* 105 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.hasTouchEvents = void 0;
function hasTouchEvents(win) {
    return 'ontouchstart' in win;
}
exports.hasTouchEvents = hasTouchEvents;

/***/ }),
/* 106 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.hasTouchPoints = void 0;
var getIsEdge_1 = __webpack_require__(107);
function hasPointerEvents(win) {
    return Boolean(win.PointerEvent);
}
function getMaxTouchPoints(win) {
    var navigator = win.navigator || {};
    var msMaxTouchPoints = navigator.msMaxTouchPoints,
        maxTouchPoints = navigator.maxTouchPoints;
    return msMaxTouchPoints || maxTouchPoints || 0;
}
function hasTouchPoints(win) {
    return hasPointerEvents(win) && getMaxTouchPoints(win) > 0 && !getIsEdge_1.getIsEdge(win);
}
exports.hasTouchPoints = hasTouchPoints;

/***/ }),
/* 107 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsEdge = void 0;
var getInternetExplorerVersion_1 = __webpack_require__(5);
var getIsEdge = function getIsEdge(win) {
    if (win === void 0) {
        win = window;
    }
    return getInternetExplorerVersion_1.getInternetExplorerVersion(win) > 11;
};
exports.getIsEdge = getIsEdge;

/***/ }),
/* 108 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isMatchingAnyPointerCoarse = void 0;
var map_1 = __webpack_require__(7);
var isMatchingMediaQuery_1 = __webpack_require__(38);
var prefixes_1 = __webpack_require__(16);
var ANY_POINTER_COARSE = map_1.map(prefixes_1.cssPrefixes, function (prefix) {
    return "(" + prefix + "any-pointer:coarse)";
}).join(',');
function isMatchingAnyPointerCoarse(win) {
    return isMatchingMediaQuery_1.isMatchingMediaQuery(win, ANY_POINTER_COARSE);
}
exports.isMatchingAnyPointerCoarse = isMatchingAnyPointerCoarse;

/***/ }),
/* 109 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isMatchingTouchEnabled = exports.TOUCH_ENABLED_QUERY = void 0;
var map_1 = __webpack_require__(7);
var isMatchingMediaQuery_1 = __webpack_require__(38);
var prefixes_1 = __webpack_require__(16);
exports.TOUCH_ENABLED_QUERY = map_1.map(prefixes_1.cssPrefixes, function (prefix) {
    return "(" + prefix + "touch-enabled)";
}).join(',');
function isMatchingTouchEnabled(win) {
    return isMatchingMediaQuery_1.isMatchingMediaQuery(win, exports.TOUCH_ENABLED_QUERY);
}
exports.isMatchingTouchEnabled = isMatchingTouchEnabled;

/***/ }),
/* 110 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getSafariVersion = void 0;
var isSafari_1 = __webpack_require__(111);
function getSafariVersion(win) {
    if (win === void 0) {
        win = window;
    }
    if (isSafari_1.isSafari && win.navigator && win.navigator.userAgent) {
        var match = win.navigator.userAgent.match(/version\/(\d+)/i);
        if (match) {
            var version = Number(match[1]);
            if (version) {
                return version;
            }
        }
    }
    return -1;
}
exports.getSafariVersion = getSafariVersion;

/***/ }),
/* 111 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isSafari = void 0;
var getIsSafari_1 = __webpack_require__(9);
exports.isSafari = getIsSafari_1.getIsSafari();

/***/ }),
/* 112 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isCssFilterBlurSupported = exports.isCssAnimationSupported = exports.isCssTransitionSupported = exports.isCssTransformSupported = exports.isCssFlexSupported = void 0;
var testProperty_1 = __webpack_require__(39);
exports.isCssFlexSupported = testProperty_1.testProperty('flex');
exports.isCssTransformSupported = testProperty_1.testProperty('transform');
exports.isCssTransitionSupported = testProperty_1.testProperty('transition');
exports.isCssAnimationSupported = testProperty_1.testProperty('animation');
exports.isCssFilterBlurSupported = testProperty_1.testProperty('filter:blur(2px)');

/***/ }),
/* 113 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsLongUrlSupported = void 0;
var getisIE_1 = __webpack_require__(114);
var getIsLongUrlSupported = function getIsLongUrlSupported(win) {
    if (win === void 0) {
        win = window;
    }
    return !getisIE_1.getIsIE(win);
};
exports.getIsLongUrlSupported = getIsLongUrlSupported;

/***/ }),
/* 114 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsIE = void 0;
var getInternetExplorerVersion_1 = __webpack_require__(5);
var getIsIE = function getIsIE(win) {
    if (win === void 0) {
        win = window;
    }
    return getInternetExplorerVersion_1.getInternetExplorerVersion(win) !== -1;
};
exports.getIsIE = getIsIE;

/***/ }),
/* 115 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsPostMessageSupported = void 0;
var getInternetExplorerVersion_1 = __webpack_require__(5);
var isOperaMini_1 = __webpack_require__(36);
var isFunction_1 = __webpack_require__(8);
var getIsPostMessageSupported = function getIsPostMessageSupported(win) {
    if (win === void 0) {
        win = window;
    }
    return isFunction_1.isFunction(win.postMessage) && getInternetExplorerVersion_1.getInternetExplorerVersion(win) === -1 && !isOperaMini_1.isOperaMini(win);
};
exports.getIsPostMessageSupported = getIsPostMessageSupported;

/***/ }),
/* 116 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getIsIEQuirks = exports.getIsQuirks = void 0;
var getInternetExplorerVersion_1 = __webpack_require__(5);
var getIsQuirks = function getIsQuirks(win) {
    if (win === void 0) {
        win = window;
    }
    return win.document.compatMode === 'BackCompat';
};
exports.getIsQuirks = getIsQuirks;
var getIsIEQuirks = function getIsIEQuirks(win) {
    if (win === void 0) {
        win = window;
    }
    var ieVersion = getInternetExplorerVersion_1.getInternetExplorerVersion(win);
    return ieVersion > 0 && (win.document.documentMode === 5 || ieVersion !== 10 && exports.getIsQuirks(win));
};
exports.getIsIEQuirks = getIsIEQuirks;

/***/ }),
/* 117 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.getBrowserName = void 0;
var getUserAgent_1 = __webpack_require__(3);
var getBrowserName = function getBrowserName(win) {
    if (win === void 0) {
        win = window;
    }
    var ua = getUserAgent_1.getUserAgent(win);
    var re = ua.match(/(opera|chrome|safari|firefox|ucbrowser|msie|trident(?=\/))\/?\s*(\d+)/i) || [];
    if (/trident/i.test(re[1])) {
        return 'MSIE';
    }
    if (re[1] === 'Chrome') {
        var edgeRegex = ua.match(/\b(OPR|Edge|YaBrowser)\/(\d+)/);
        if (edgeRegex !== null) {
            return edgeRegex[1].replace('OPR', 'Opera');
        }
    }
    return re[1];
};
exports.getBrowserName = getBrowserName;

/***/ }),
/* 118 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


Object.defineProperty(exports, "__esModule", { value: true });
exports.isNativeSendBeaconSupported = void 0;
var checkNativeCode_1 = __webpack_require__(4);
var isNativeSendBeaconSupported = function isNativeSendBeaconSupported(window) {
    return checkNativeCode_1.checkNativeCode(window.navigator.sendBeacon) || false && !false;
};
exports.isNativeSendBeaconSupported = isNativeSendBeaconSupported;

/***/ })
/******/ ])));