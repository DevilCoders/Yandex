/* eslint quotes: ["error", "single", {"avoidEscape": true}] */
const {URL} = require('url');
const os = require('os');
const yastaticCSP = require('csp-preset-yastatic');
const backendUrl = process.env.BACKEND_API_URL || 'https://preprod.aabadmin.yandex.ru/';
const backendHostName = new URL(backendUrl).hostname;

module.exports = {

    permissions: {
        INTERFACE_SEE: 'interface_see',
        SERVICE_CREATE: 'service_create',
        SERVICE_SEE: 'service_see',
        CONFIG_MARK_TEST: 'config_mark_test',
        CONFIG_MARK_ACTIVE: 'config_mark_active',
        CONFIG_CREATE: 'config_create',
        HIDDEN_FIELD_SEE: 'hidden_fields_see',
        HIDDEN_FIELDS_UPDATE: 'hidden_fields_update',
        TOKEN_UPDATE: 'token_update',
        AUTH_GRANT: 'auth_grant',
        CONFIG_MODERATE: 'config_moderate',
        SERVICE_COMMENT: 'service_comment',
        TICKET_CREATE: 'ticket_create',
        TICKET_SEE: 'ticket_see',
        SUPPORT_PRIORITY_SWITCH: 'support_priority_switch'
    },

    api: {
        url: backendUrl,
        version: 'v1'
    },

    app: {
        baseUrl: ''
    },

    blackbox: {
        api: process.env.BLACKBOX_HOST || 'blackbox-mimino.yandex.net',
        params: {
            regname: 'yes'
        }
    },

    csp: {
        policies: {
            'default-src': ["'none'"],
            // TODO: не забыть про self
            'script-src': ["'self'", "'unsafe-eval'", "'unsafe-inline'", '%nonce%', 'mc.yandex.ru', 'social.yandex.ru', 'an.yandex.ru', 'pass.yandex.ru', 'pass.yandex-team.ru'],
            'style-src': ["'self'", "'unsafe-inline'", 'mc.yandex.ru'],
            'font-src': ["'self'"],
            'img-src': ["'self'", 'data:', 'avatars.yandex.net', 'avatars.mds.yandex.net', 'avatars.mdst.yandex.net', 'mc.yandex.ru', 'yandex.ru', 'jing.yandex-team.ru', 'http://argus.s3.mds.yandex.net'],
            'connect-src': ["'self'", 'mc.yandex.ru', 'avatars.mds.yandex.net', 'avatars.mdst.yandex.net', 'yandex.ru', 'api.antiblock.yandex.ru', 'api.antiadb.yandex.ru', '*.qams.yandex.ru', backendHostName],
            'frame-src': ["'self'", 'storage.mdst.yandex.net', 'storage.mds.yandex.net', 'yastatic.net']
        },
        presets: [yastaticCSP.script, yastaticCSP.style, yastaticCSP.font, yastaticCSP.image],
        useDefaultReportUri: true,
        serviceName: 'antiadb-front'
    },

    frameguard: {
        whitelist: [
            '.yandex.ru',
            '.yandex.by',
            '.yandex.net',
            '.yandex.com.tr',
            '.yandex.ua',
            '.yandex.kz',
            '.yandex.com',
            '.yandex-team.ru' // moderation lives here
        ]
    },

    langdetect: {
        availableLanguages: {
            ru: ['ru', 'en'],
            by: ['ru', 'en'],
            kz: ['ru', 'en'],
            ua: ['ru', 'uk'],
            com: ['ru', 'en'],
            net: ['ru', 'en'],
            'com.tr': ['tr', 'en']
        },

        defaultLang: 'ru'
    },

    build: {
        languages: {
            ru: true,
            en: true
        }
    },

    geobase: {
        mock: {
            is_yandex: true,
            gid_is_trusted: true,
            location: {lat: 55.755768, lon: 37.617671},
            point_id: -1,
            precision: 2,
            precision_by_ip: 0,
            region_id: 213,
            region_id_by_gp: -1,
            region_id_by_ip: 0,
            should_update_cookie: false,
            suspected_region_id: -1
        }
    },

    logs: {
        name: 'antiadb-front',
        streams: [{
            level: 'info',
            stream: process.stdout
        }],
        excludes: ['*']
    },

    render: {
        entry: './app/build.js',
        webpackConfig: '../webpack.config.js',
        publicPath: '/'
    },

    server: {
        host: os.hostname(),
        port: process.env.PORT || 8080
    },

    connections: {
        apiRequestTimeout: 60 * 1000 // ms
    },

    statics: {
        // FIXME: that has same prefix as in /nginx/nginx.conf
        host: '/static',
        dir: './static'
    },

    /**  serve static via this nodejs even in production
     *
    statics: {
       host: `//yastatic.net/q/react-demo/${process.env.APP_VERSION}/static`,
       dir: './static'
    },
    */
    uatraits: {
        mock: {
            isBrowser: true
        }
    }
};
