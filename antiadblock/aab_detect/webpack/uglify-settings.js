/**
 * Общие настройки webpack
 * @file
 */

const webpack = require('webpack');

const standardProps = require('uglify-js/tools/domprops');
module.exports = new webpack.optimize.UglifyJsPlugin({
    include: /\.min\.js$/,
    comments: false,
    mangle: {
        props: {
            // regex: /setCookieWithConfig/g
            reserved: [
                // При использовании reserved перестают работать стандартные свойства.
                // Подключаем их руками
                ...standardProps,
                // result
                'blocker',
                'blocked',
                'fakeDetect',
                // global config
                'pid',
                'cookieMatching',
                'deprecatedCookies',
                'cookieName',
                'cookieDomain',
                'detect',
                'encode',
                'extuidCookies',
                'replaceClasses',
                'cookieTTL',
                'cryptedUidUrl',
	            'cryptedUidCookie',
	            'cryptedUidTTL',
                'disableShadow',
                'treeProtection',
                // domain type
                'type',
                'types',
                'list',
                // matching
                'publisherTag',
                'publisherKey',
                'redirectUrl',
                'imageUrl',
                'expiredDate',
                // encode
                'key',
                'seed',
                'urlPrefix',
                'trailingSlash',
                // detect
                'elements',
                'links',
                'custom',
                'iframes',
                'trusted',
                // links
                'src',
                // user config
                'cookie',
                'time',
                'expires',
                'context',
                'callback',
                'dbltsr',
                'debug',
                'force',
                // treeProtection
                'enabled',
                // sendBamboozled
                'event',
                'host',
                'v',
                'ab',
                'data',
                'action',
                'reason',
                'invertedCookieEnabled',
                // logs
                'typeOfMatching',
                'date',
                'browser',
                'matchingType',
                'device',
                'element',
                'sid',
                'service',
                'eventName',
                'eventType',
                'topLocation',
                'topReferrer',
                'currentScriptSrc',
                'styles',
                'additional',
                'inframe',
                // navigator. connection
                'effectiveType',
                'connection',
                'mozConnection',
                'webkitConnection',
                'online',
                // forcecry
                'forcecry',
                'percent',
                // encode functions
                'isEncodedUrl',
                'encodeUrl',
                'encodeCSS',
                'decodeUrl',
                'fn',
                // base64
                'addEquals',
                'decode',
                'encodeUInt8String',
                'decodeUInt8String',
                // cookie
                'secure',
                'sameSite',
                // functions
                'DocumentTouch',
                'detectLogPortion',
                'additionalParams',
                'getBrowserName',
                'testProperty',
                'forOwn',
                'getPixelRatio',
                'once',
                'passive',
                'capture',
                'fail',
            ]
        }
    },
    minimize: true
});
