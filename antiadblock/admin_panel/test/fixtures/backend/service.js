export const SERVICE = {
    id: 123,
    name: 'ololo.ru',
    status: 'ok',
    comment: '',
    mobile_monitorings_enabled: true,
    monitorings_enabled: true
};

export const SERVICE_CONFIG = {
    comment: 'stand alone a config',
    date: '2018-01-10T11:12:13',
    id: 456,
    status: 'inactive'
};

export const SERVICE_CONFIGS = {
    items: [
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 234,
            statuses: [{status: 'preview', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 235,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 236,
            statuses: [{status: 'preview', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 237,
            statuses: [{status: 'preview', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 238,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 239,
            statuses: []
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 240,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 241,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 242,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 243,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 244,
            statuses: [{status: 'preview', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 245,
            statuses: []
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 246,
            statuses: []
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 247,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 248,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 249,
            statuses: [{status: 'preview', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 250,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 251,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 252,
            statuses: [{status: 'active', comment: ''}]
        },
        {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 253,
            statuses: [{status: 'active', comment: ''}]
        }
    ],
    total: 60
};

export const SETUP = {
    token: 'AABBCCDDEEFF....',
    js_inline: '(func () { /* here goes awesome JS code */})("<PARTNER-ID>")',
    nginx_config: 'here \n goes \n config'
};

export const SERVICE_AUDIT = {
    items: [
        {
            action: 'config_active',
            date: 'Mon, 12 Feb 2018 16:19:23 GMT',
            id: 1,
            params: {
                config_id: 3,
                config_comment: 'wertwer',
                old_config_id: 1
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        {
            action: 'config_test',
            date: 'Mon, 12 Feb 2018 16:46:39 GMT',
            id: 2,
            params: {
                config_id: 1,
                config_comment: 'Initial config',
                old_config_id: null
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        {
            action: 'config_active',
            date: 'Mon, 12 Feb 2018 16:51:53 GMT',
            id: 3,
            params: {
                config_id: 1,
                config_comment: 'Initial config',
                old_config_id: 3
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        {
            action: 'config_test',
            date: 'Mon, 12 Feb 2018 16:56:02 GMT',
            id: 4,
            params: {
                config_id: 3,
                config_comment: 'wertwer',
                old_config_id: null
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        {
            action: 'config_active',
            date: 'Mon, 12 Feb 2018 16:57:02 GMT',
            id: 5,
            params: {
                config_id: 3,
                config_comment: 'wertwer',
                old_config_id: 1
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        {
            action: 'config_test',
            date: 'Mon, 12 Feb 2018 17:18:51 GMT',
            id: 6,
            params: {
                config_id: 1,
                config_comment: 'Initial config',
                old_config_id: null
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        }
    ],
    total: 6
};

export const SERVICE_CONFIG_ARCHIVED = {
    archived: true,
    comment: 'test stand alone config',
    created: 'Fri, 06 Apr 2018 13:48:14 GMT',
    creator_id: 393530940,
    data: {},
    id: 345,
    parent_id: 123,
    service_id: 'yandex_afisha',
    status: 'inactive'
};
