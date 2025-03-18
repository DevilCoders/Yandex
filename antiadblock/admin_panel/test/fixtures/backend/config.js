export const CONFIG_ACTIVE = {
    comment: 'made a config',
    date: '2018-01-10T11:12:13',
    id: 234,
    statuses: [{status: 'active', comment: ''}],
    data: {
        field1: 'value1',
        field2: 'value2',
        field3: 'value3'
    }
};

export const CONFIG_APPROVED = {
    comment: 'made a config',
    date: '2018-01-10T11:12:13',
    id: 239,
    statuses: [{status: 'approved', comment: ''}],
    data: {
        field1: 'value1',
        field2: 'value2',
        field3: 'value3'
    }
};

export const CONFIG_DECLINED = {
    comment: 'made a config',
    date: '2018-01-10T11:12:13',
    id: 239,
    statuses: [{status: 'declined', comment: 'decline message'}],
    data: {
        field1: 'value1',
        field2: 'value2',
        field3: 'value3'
    }
};

export const CONFIG_PREVIEW = {
    comment: 'made a config',
    date: '2018-01-10T11:12:13',
    id: 236,
    statuses: [{status: 'preview', comment: ''}],
    data: {
        field1: 'value1',
        field2: 'value2',
        field3: 'value3'
    }
};

export const CONFIG_ACTUAL_ACTIVE = {
    created: 'Mon, 12 Feb 2018 16:18:21 GMT',
    creator_id: 204409962,
    data: {
        CM_TYPE: 0,
        CRYPT_SECRET_KEY: 'phoohouWun3Hu9maciePhukoh4uth4ie',
        CRYPT_URL_RE: [
            'avatar\\.mds\\.yandex\\.net/get-afishanew/.*?'
        ],
        ENCRYPTION_STEPS: [
            0,
            1,
            2,
            3
        ],
        EXCLUDE_COOKIE_FORWARD: [],
        EXTUID_COOKIE_NAMES: [
            'yandexuid'
        ],
        EXTUID_TAG: 'yandex-afisha',
        INTERNAL: true,
        PARTNER_TOKENS: [
            'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJBQUIiLCJpYXQiOjE1MTE4Njk2NDgsInN1YiI6InlhbmRleF9hZmlzaGEiLCJleHAiOjE1NDM0MTY0NDh9.S6mXAooufGoYUHg9m6GZiNZVxR0XRB1lmKUTzlQUoEg'
        ],
        PROXY_URL_RE: [
            '(?:(?:beta|afisha-frontend\\.\\w+\\.spec(?:-new)?)\\.)?afisha(?:\\.(?:dev|qa|prestable|tst|load))?\\.yandex\\.(?:ru|net|ua|by|kz|com)/.*?'
        ],
        PUBLISHER_SECRET_KEY: 'eyJhbGciOi',
        TEST_DATA: {
            invalid_domains: [
                'afisha.ru',
                'afisha.yandex',
                'dev.tst.afisha.yandex.ru',
                'qa.afisha.yandex.ru',
                'afisha-yandex.ru'
            ],
            valid_domains: [
                'afisha.yandex.ru',
                'afisha.yandex.net',
                'afisha.prestable.yandex.ru',
                'afisha.qa.yandex.ru',
                'afisha.tst.yandex.ru',
                'afisha.dev.yandex.ru',
                'beta.afisha.dev.yandex.ru',
                'afisha-frontend.yunnii.spec.afisha.dev.yandex.ru',
                'afisha-frontend.yunnii.spec-new.afisha.dev.yandex.ru',
                'afisha.load.yandex.ru'
            ]
        }
    },
    id: 3,
    comment: 'wertwer',
    parent_id: null,
    service_id: 'yandex_afisha',
    statuses: [{status: 'active', comment: ''}]
};
