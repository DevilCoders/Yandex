import {SERVICE, SERVICE_CONFIG, SERVICE_CONFIGS, SERVICE_AUDIT} from '../backend/service';

export const START_SERVICE_LOADING = {
    type: 'START_SERVICE_LOADING',
    serviceId: 123
};

export const END_SERVICE_LOADING = {
    type: 'END_SERVICE_LOADING',
    serviceId: 123,
    service: SERVICE
};

export const START_SERVICE_CONFIG_LOADING = {
    type: 'START_SERVICE_CONFIG_LOADING',
    serviceId: 123,
    configId: 456
};

export const END_SERVICE_CONFIG_LOADING = {
    type: 'END_SERVICE_CONFIG_LOADING',
    serviceId: 123,
    configId: 456,
    config: SERVICE_CONFIG
};

export const START_SERVICE_CONFIGS_LOADING = {
    type: 'START_SERVICE_CONFIGS_LOADING',
    labelId: 123,
    offset: 0,
    limit: 20
};

export const END_SERVICE_CONFIGS_LOADING = {
    type: 'END_SERVICE_CONFIGS_LOADING',
    labelId: 123,
    configs: SERVICE_CONFIGS
};

export const START_SERVICE_CONFIG_APPLYING = {
    type: 'START_SERVICE_CONFIG_APPLYING',
    labelId: 123,
    configId: 234,
    options: {
        target: 'active',
        oldConfigId: 235
    }
};

export const END_SERVICE_CONFIG_APPLYING = {
    type: 'END_SERVICE_CONFIG_APPLYING',
    labelId: 123,
    configId: 234,
    options: {
        target: 'active',
        oldConfigId: 235
    }
};

export const START_SERVICE_SETUP_LOADING = {
    type: 'START_SERVICE_SETUP_LOADING',
    serviceId: 123
};

export const END_SERVICE_SETUP_LOADING = {
    type: 'END_SERVICE_SETUP_LOADING',
    serviceId: 123,
    setup: {
        token: 'AABBCCDDEEFF....',
        js_inline: '(func () { /* here goes awesome JS code */})("<PARTNER-ID>")',
        nginx_config: 'here \n goes \n config'
    }
};

export const RESET_SERVICE_AUDIT = {
    type: 'RESET_SERVICE_AUDIT',
    serviceId: 'yandex_afisha'
};

export const START_SERVICE_AUDIT_LOADING = {
    type: 'START_SERVICE_AUDIT_LOADING',
    serviceId: 'yandex_afisha',
    offset: 0,
    limit: 20
};

export const END_SERVICE_AUDIT_LOADING = {
    type: 'END_SERVICE_AUDIT_LOADING',
    serviceId: 'yandex_afisha',
    audit: SERVICE_AUDIT
};

export const START_SERVICE_CONFIG_ARCHIVED_SETTING = {
    type: 'START_SERVICE_CONFIG_ARCHIVED_SETTING',
    serviceId: 'yandex_afisha',
    configId: 345,
    archived: true
};

export const END_SERVICE_CONFIG_ARCHIVED_SETTING = {
    type: 'END_SERVICE_CONFIG_ARCHIVED_SETTING',
    serviceId: 'yandex_afisha',
    configId: 345,
    archived: true
};

export const START_SERVICE_COMMENT = {
    type: 'START_SERVICE_COMMENT',
    serviceId: 123
};

export const END_SERVICE_COMMENT = {
    type: 'END_SERVICE_COMMENT',
    serviceId: 123,
    comment: 'comment'
};
