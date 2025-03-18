export const SERVICE_EMPTY = {
    service: null,
    setup: {
        loaded: false
    },
    comment: '',
    configs: {
        byId: {},
        items: [],
        loaded: false,
        offset: 0,
        total: 0
    },
    applyingConfig: {
        progress: false,
        id: null,
        target: null // preview, active
    },
    loaded: false
};

export const SERVICE = {
    service: {
        id: 123,
        name: 'ololo.ru',
        status: 'ok',
        comment: ''
    },
    setup: {
        loaded: false
    },
    configs: {
        byId: {},
        items: [],
        loaded: false,
        offset: 0,
        total: 0
    },
    applyingConfig: {
        progress: false,
        id: null,
        target: null
    },
    moderatingConfig: {
        progress: false,
        id: null,
        approved: null,
        comment: null
    },
    loaded: true
};

export const CONFIGS = {
    byId: {
        234: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 234,
            statuses: [{status: 'active', comment: ''}]
        },
        235: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 235,
            statuses: [{status: 'preview', comment: ''}]
        },
        236: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 236,
            statuses: []
        },
        237: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 237,
            statuses: [{status: 'preview', comment: ''}]
        },
        238: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 238,
            statuses: [{status: 'preview', comment: ''}]
        },
        239: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 239,
            statuses: []
        },
        240: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 240,
            statuses: [{status: 'active', comment: ''}]
        },
        241: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 241,
            statuses: [{status: 'preview', comment: ''}]
        },
        242: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 242,
            statuses: [{status: 'active', comment: ''}]
        },
        243: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 243,
            statuses: [{status: 'active', comment: ''}]
        },
        244: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 244,
            statuses: []
        },
        245: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 245,
            statuses: []
        },
        246: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 246,
            statuses: []
        },
        247: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 247,
            statuses: [{status: 'preview', comment: ''}]
        },
        248: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 248,
            statuses: [{status: 'preview', comment: ''}]
        },
        249: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 249,
            statuses: [{status: 'preview', comment: ''}]
        },
        250: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 250,
            statuses: [{status: 'active', comment: ''}]
        },
        251: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 251,
            statuses: [{status: 'active', comment: ''}]
        },
        252: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 252,
            statuses: []
        },
        345: {
            comment: 'made a config',
            date: '2018-01-10T11:12:13',
            id: 345,
            statuses: [{status: 'preview', comment: ''}]
        }
    },
    items: [
        234,
        235,
        236,
        237,
        238,
        239,
        240,
        241,
        242,
        243,
        244,
        245,
        246,
        247,
        248,
        249,
        250,
        251,
        252,
        345
    ],
    loaded: true,
    offset: 20,
    total: 60
};

export const AUDIT = {
    byId: {
        1: {
            action: 'config_active',
            date: 'Mon, 12 Feb 2018 16:19:23 GMT',
            id: 1,
            params: {
                config_comment: 'wertwer',
                config_id: 3,
                old_config_id: 1
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        2: {
            action: 'config_test',
            date: 'Mon, 12 Feb 2018 16:46:39 GMT',
            id: 2,
            params: {
                config_comment: 'Initial config',
                config_id: 1,
                old_config_id: null
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        3: {
            action: 'config_active',
            date: 'Mon, 12 Feb 2018 16:51:53 GMT',
            id: 3,
            params: {
                config_comment: 'Initial config',
                config_id: 1,
                old_config_id: 3
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
            },
        4: {
            action: 'config_test',
            date: 'Mon, 12 Feb 2018 16:56:02 GMT',
            id: 4,
            params: {
                config_comment: 'wertwer',
                config_id: 3,
                old_config_id: null
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        5: {
            action: 'config_active',
            date: 'Mon, 12 Feb 2018 16:57:02 GMT',
            id: 5,
            params: {
                config_comment: 'wertwer',
                config_id: 3,
                old_config_id: 1
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        },
        6: {
            action: 'config_test',
            date: 'Mon, 12 Feb 2018 17:18:51 GMT',
            id: 6,
            params: {
                config_comment: 'Initial config',
                config_id: 1,
                old_config_id: null
            },
            service_id: 'yandex_afisha',
            user_id: 204409962
        }
    },
    items: [
        1,
        2,
        3,
        4,
        5,
        6
    ],
    loaded: true,
    offset: 6,
    total: 6
};

export const SCHEMA = [
    {
        group_name: 'TESTGROUP',
        title: 'testgroup-title',
        hint: 'testgroup-hint',
        items: [
            {
                key: {
                    default: [],
                    name: 'ACCEL_REDIRECT_URL_RE',
                    type_schema: {
                        children: {
                            placeholder: 'regexp-placeholder',
                            type: 'regexp'
                        },
                        hint: 'accel-redirect-url-re-hint',
                        title: 'accel-redirect-url-re-title',
                        type: 'array'
                    }
                }
            },
            {
                key: {
                    default: true,
                    name: 'ADB_ENABLED',
                    type_schema: {
                        hint: 'adb-enabled-hint',
                        placeholder: {
                            placeholder_off: 'adb-enabled-off-placeholder',
                            placeholder_on: 'adb-enabled-on-placeholder'
                        },
                        title: 'adb-enabled-title',
                        type: 'bool'
                    }
                }
            },
            {
                key: {
                    default: false,
                    name: 'ADFOX_DEBUG',
                    type_schema: {
                        hint: 'adfox-debug-hint',
                        placeholder: {
                            placeholder_off: 'adfox-debug-off-placeholder',
                            placeholder_on: 'adfox-debug-on-placeholder'
                        },
                        title: 'adfox-debug-title',
                        type: 'bool'
                    }
                }
            }
        ]
    }
];

export const SERVICE_WITH_CONFIGS = {
    service: {
        id: 123,
        name: 'ololo.ru',
        status: 'ok',
        comment: ''
    },
    setup: {
        loaded: false
    },
    configs: CONFIGS,
    audit: AUDIT,
    schema: SCHEMA,
    applyingConfig: {
        progress: false,
        id: null,
        target: null
    },
    loaded: true
};

export const SERVICES_EMPTY = {
    items: [],
    loading: false,
    loaded: false
};

export const CONFIG_PREVIEW_EMPTY = {
    visible: false,
    id: null,
    serviceId: null,
    configData: null
};

export const CONFIRM_DIALOG_EMPTY = {
    visible: false,
    message: null,
    callback: null
};

export const ERRORS = {
    byId: {
        global: ['global message'],
        id1: [
            'message 1',
            'message 2'
        ],
        id2: [
            'message 3',
            'message 4'
        ]
    }
};

export const CONFIG_APPLYING_EMPTY = {
    visible: false,
    id: null,
    serviceId: null,
    configData: null,
    activeConfigData: null,
    loaded: false
};

export const CONFIG_APPLYING_OPENED = {
    type: 'OPEN_CONFIG_APPLYING',
    serviceId: 123,
    id: 236,
    configData: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 236,
        statuses: [{status: 'preview', comment: ''}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    }
};

export const CONFIG_APPLYING_LOADING = {
    visible: true,
    id: 236,
    serviceId: 123,
    configData: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 236,
        statuses: [{status: 'preview', comment: ''}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    },
    activeConfigData: null,
    loaded: false
};

export const CONFIG_APPLYING_LOADED = {
    visible: true,
    id: 236,
    serviceId: 123,
    configData: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 236,
        statuses: [{status: 'preview', comment: ''}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    },
    activeConfigData: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 234,
        statuses: [{status: 'active', comment: ''}]
    },
    loaded: true
};

export const CONFIG_DIFF_EMPTY = {
    visible: false,
    id: null,
    oldId: null,
    serviceId: null,
    configsData: {},
    loaded: false
};

export const CONFIG_DIFF_OPENED = {
    modalVisible: true,
    id: 1,
    secondId: 3,
    serviceId: 'yandex_afisha',
    configsData: {}
};

export const METRIC = {
    'test.second.service': {
        http_codes: {
            range: 330,
            loading: false,
            loaded: true,
            data: {
                1524759180000: {
                    200: 2368,
                    204: 85,
                    301: 182
                },
                1524759240000: {
                    200: 2797,
                    204: 100,
                    301: 232
                }
            }
        },
        bamboozled_by_app: {
            range: 330,
            loading: true,
            loaded: false
        },
        bamboozled_by_bro: {
            range: 330,
            loading: false,
            loaded: false
        }
    },
    'test.service': {
        http_codes: {
            range: 10,
            loading: false,
            loaded: true,
            data: {
                1524759180000: {
                    200: 2368,
                    204: 85,
                    301: 182,
                    302: 11,
                    304: 1,
                    404: 173
                },
                1524759240000: {
                    200: 2797,
                    204: 100,
                    301: 232,
                    302: 20,
                    304: 7,
                    404: 206
                }
            }
        }
    }
};

export const ROOT_STATE = {
    service: SERVICE_EMPTY,
    services: SERVICES_EMPTY,
    configPreview: CONFIG_PREVIEW_EMPTY,
    configDiffModal: CONFIG_DIFF_EMPTY,
    configApplying: CONFIG_APPLYING_EMPTY,
    metric: METRIC,
    errors: ERRORS
};

export const START_SERVICE_CONFIG_APPROVING = {
    type: 'START_SERVICE_CONFIG_MODERATING',
    serviceId: 123,
    configId: 239,
    approved: 'approved',
    comment: ''
};

export const END_SERVICE_CONFIG_APPROVING = {
    type: 'END_SERVICE_CONFIG_MODERATING',
    serviceId: 123,
    configId: 239,
    approved: 'approved',
    comment: '',
    config: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 239,
        statuses: [{status: 'approved', comment: ''}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    }
};

export const START_SERVICE_CONFIG_DECLINING = {
    type: 'START_SERVICE_CONFIG_MODERATING',
    serviceId: 123,
    configId: 239,
    approved: 'declined',
    comment: 'decline message'
};

export const END_SERVICE_CONFIG_DECLINING = {
    type: 'END_SERVICE_CONFIG_MODERATING',
    serviceId: 123,
    configId: 239,
    approved: 'declined',
    comment: 'decline message',
    config: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 239,
        statuses: [{status: 'declined', comment: 'decline message'}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    }
};

export const BEFORE_SEARCH = {
    byId: {},
    items: [],
    loaded: false,
    total: 0
};

export const START_SEARCH = {
    type: 'START_SEARCH',
    loaded: false,
    items: [],
    pattern: 'test',
    active: true
};

export const END_SEARCH = {
    type: 'END_SEARCH',
    loaded: true,
    pattern: 'test',
    items: [],
    byId: {},
    offset: 0,
    total: 0
};
