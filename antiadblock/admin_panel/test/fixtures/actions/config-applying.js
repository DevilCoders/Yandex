export const OPEN_CONFIG_APPLYING = {
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
    },
    target: 'active'
};

export const CLOSE_CONFIG_APPLYING = {
    type: 'CLOSE_CONFIG_APPLYING',
    id: 236
};

export const START_ACTIVE_CONFIG_LOADING = {
    type: 'START_ACTIVE_CONFIG_LOADING',
    labelId: 123,
    target: 'active'
};

export const END_ACTIVE_CONFIG_LOADING = {
    type: 'END_ACTIVE_CONFIG_LOADING',
    labelId: 123,
    config: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 234,
        statuses: [{status: 'active', comment: ''}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    },
    target: 'active'
};
