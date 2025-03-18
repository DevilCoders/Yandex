export const OPEN_CONFIG_PREVIEW = {
    type: 'OPEN_CONFIG_PREVIEW',
    id: 234,
    serviceId: 123,
    configData: {
        comment: 'made a config',
        date: '2018-01-10T11:12:13',
        id: 234,
        statuses: [{status: 'active', comment: ''}],
        data: {
            field1: 'value1',
            field2: 'value2',
            field3: 'value3'
        }
    }
};

export const CLOSE_CONFIG_PREVIEW = {
    type: 'CLOSE_CONFIG_PREVIEW',
    id: 234
};
