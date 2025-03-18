import configApi from 'app/api/config';
import {setGlobalErrors} from 'app/actions/errors';

export const OPEN_CONFIG_APPLYING = 'OPEN_CONFIG_APPLYING';
export const CLOSE_CONFIG_APPLYING = 'CLOSE_CONFIG_APPLYING';
export const START_ACTIVE_CONFIG_LOADING = 'START_ACTIVE_CONFIG_LOADING';
export const END_ACTIVE_CONFIG_LOADING = 'END_ACTIVE_CONFIG_LOADING';

export function openConfigApplying(serviceId, id, configData, target) {
    return {
        type: OPEN_CONFIG_APPLYING,
        serviceId,
        id,
        configData,
        target
    };
}

export function closeConfigApplying(id) {
    return {
        type: CLOSE_CONFIG_APPLYING,
        id
    };
}

export function fetchCurrentConfig(labelId, target) {
    return dispatch => {
        dispatch(startActiveConfigLoading(labelId, target));

        return configApi.fetchCurrentConfig(labelId, target).then(config => {
            return dispatch(endActiveConfigLoading(labelId, target, config));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startActiveConfigLoading(labelId, target) {
    return {
        type: START_ACTIVE_CONFIG_LOADING,
        labelId,
        target
    };
}

export function endActiveConfigLoading(labelId, target, config) {
    return {
        type: END_ACTIVE_CONFIG_LOADING,
        labelId,
        target,
        config
    };
}
