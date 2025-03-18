const HOST = 'preprod.aabadmin.yandex.ru';

// бек для тестинга новых фичей
// const HOST = 'test.aabadmin.yandex.ru';
// const HOST = 'api-aabadmin44.n.yandex-team.ru';

module.exports = {

    api: {
        url: `https://${HOST}/`,
        version: 'v1'
    },

    csp: {
        useDefaultReportUri: false
    },

    build: {
        languages: {
            ru: true,
            en: false,
            tr: false
        }
    },

    statics: {
        host: '/static'
    },

    render: {
        hot: true
    },

    server: {
        host: 'localhost',
        port: 8080
    },

    uatraits: {
        mock: {
            isBrowser: true
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

    langdetect: {
        mock: {
            id: 'ru',
            name: 'Ru'
        }
    },

    blackbox: {
        api: 'display-blackbox.qloud.yandex.ru',
        // see server/middlewares/blackbox.js
        mock: true
    }
};
