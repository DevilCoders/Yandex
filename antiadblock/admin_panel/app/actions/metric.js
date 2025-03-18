import serviceApi from 'app/api/service';

export const START_METRIC_LOADING = 'START_METRIC_LOADING';
export const END_METRIC_LOADING = 'END_METRIC_LOADING';

export function fetchMetric(serviceId, name, range) {
    return dispatch => {
        dispatch(startMetricLoading(serviceId, name, range));

        return serviceApi.fetchServiceMetric(serviceId, name, range).then(data => {
            return dispatch(endMetricLoading(serviceId, name, range, data, true));
        }, () => {
            return dispatch(endMetricLoading(serviceId, name, range, []));
        });
    };
}

export function startMetricLoading(serviceId, name, range) {
    return {
        type: START_METRIC_LOADING,
        serviceId,
        name,
        range
    };
}

export function endMetricLoading(serviceId, name, range, data, loaded) {
    return {
        type: END_METRIC_LOADING,
        serviceId,
        name,
        range,
        data,
        loaded
    };
}
