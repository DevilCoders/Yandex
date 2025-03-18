module.exports = {

    server: {
        host: 'react.local.yandex.ru',
        port: 8000
    },

    blackbox: {
        api: 'display-blackbox.qloud.yandex.ru',
        mock: {
            error: 'OK',
            country: 'ru',
            lang: 'ru',
            fio: 'Pupkin Vasily',
            login: 'display-test',
            status: 'VALID',
            uid: '394759598',
            havePassword: true,
            haveHint: true,
            karma: 0,
            age: 8,
            ttl: '5',
            displayName: 'Display-test',
            avatar: {
                default: '0/0-0',
                empty: true
            },
            raw: {}
        }
    }
};
