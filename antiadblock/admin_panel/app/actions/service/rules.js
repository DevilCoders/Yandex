import serviceApi from 'app/api/service';
import {setGlobalErrors} from 'app/actions/errors';

export const START_SERVICE_RULES_LOADING = 'START_SERVICE_RULES_LOADING';
export const END_SERVICE_RULES_LOADING = 'END_SERVICE_RULES_LOADING';

export function fetchServiceRules(serviceId, tags) {
    return dispatch => {
        dispatch(startServiceRulesLoading(serviceId));

        return serviceApi.fetchServiceRules(serviceId, tags).then(data => {
            return dispatch(endServiceRulesLoading(serviceId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function endServiceRulesLoading(serviceId, data) {
    return {
        type: END_SERVICE_RULES_LOADING,
        serviceId,
        data
    };
}

export function startServiceRulesLoading(serviceId) {
    return {
        type: START_SERVICE_RULES_LOADING,
        serviceId
    };
}
