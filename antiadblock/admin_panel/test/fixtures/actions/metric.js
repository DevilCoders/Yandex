import {METRIC_HTTP_CODES} from '../backend/metric';

export const START_METRIC_LOADING = {
    type: 'START_METRIC_LOADING',
    serviceId: 'test.service',
    name: 'http_codes',
    range: 336
};

export const END_METRIC_LOADING = {
    type: 'END_METRIC_LOADING',
    serviceId: 'test.service',
    name: 'http_codes',
    range: 336,
    data: METRIC_HTTP_CODES,
    loaded: true
};
