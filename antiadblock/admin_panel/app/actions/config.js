import configApi from 'app/api/config';
import {setGlobalErrors} from 'app/actions/errors';

import {antiadbUrl} from 'app/lib/url';
import redirect from 'app/lib/redirect';
import {getStatus} from 'app/lib/config-editor';

export const START_FETCH_CONFIG_LOADING = 'START_FETCH_CONFIG_LOADING';
export const END_FETCH_CONFIG_LOADING = 'END_FETCH_CONFIG_LOADING';
export const START_CONFIG_SAVING = 'START_CONFIG_SAVING';
export const END_CONFIG_SAVING = 'END_CONFIG_SAVING';

function prepareValidation(errors = []) {
    return errors.reduce((validation, error) => {
        switch (error.path[0]) {
            case 'comment':
                validation.comment.push(error);
                break;
            case 'data':
                if (!validation.data[error.path[1]]) {
                    validation.data[error.path[1]] = [];
                }

                validation.data[error.path[1]].push(error);
                break;
            default:
                break;
        }

        return validation;
    }, {
        comment: [],
        data: {}
    });
}

export function fetchConfigById(configId, status, expId) {
    return dispatch => {
        dispatch(startConfigLoading(configId));

        return configApi.fetchConfig(configId, status, expId).then(data => {
            return dispatch(endConfigLoading(configId, data));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function saveConfig(serviceId, labelId, parentId, comment, data, dataSettings) {
    return dispatch => {
        dispatch(startConfigSaving());

        return configApi.saveConfig(labelId, parentId, comment, data, dataSettings).then(() => {
            redirect(antiadbUrl(`/service/${serviceId}/label/${labelId}/`));
        }, error => {
            if (error.status === 400) {
                const properties = error.response.properties;
                const validation = prepareValidation(properties);

                return dispatch(endConfigSaving(validation));
            }
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startConfigSaving() {
    return {
        type: START_CONFIG_SAVING
    };
}

export function endConfigSaving(validation) {
    return {
        type: END_CONFIG_SAVING,
        validation
    };
}

export function startConfigLoading(configId) {
    return {
        type: START_FETCH_CONFIG_LOADING,
        configId
    };
}

function getConfigStatus(data) {
    const statuses = (data.statuses && data.statuses[0] && data.statuses[0].status && data.statuses.map(item => item.status)) || [];

    return getStatus(statuses, data.exp_id);
}

export function endConfigLoading(configId, data) {
    return {
        type: END_FETCH_CONFIG_LOADING,
        configId,
        data: {
            ...data,
            dataCurrent: data.data,
            status: getConfigStatus(data)
        }
    };
}
