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
