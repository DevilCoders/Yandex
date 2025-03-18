import servicesApi from 'app/api/services';
import {setGlobalErrors} from 'app/actions/errors';

export const START_SEARCH = 'START_SEARCH';
export const END_SEARCH = 'END_SEARCH';

export function searchConfigs(pattern, offset, limit, active) {
    return dispatch => {
        dispatch(startSearchConfigs(pattern, offset, limit, active));

        return servicesApi.searchConfigs(pattern, offset, limit, active).then(configs => {
            return dispatch(endSearchConfigs(pattern, configs));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startSearchConfigs(pattern, offset, limit, active) {
    return {
        type: START_SEARCH,
        pattern,
        offset,
        limit,
        active
    };
}

export function endSearchConfigs(pattern, configs) {
    return {
        type: END_SEARCH,
        pattern,
        configs
    };
}
