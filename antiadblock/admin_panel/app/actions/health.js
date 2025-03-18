import serviceApi from 'app/api/service';
import {setGlobalErrors, setErrors, clearErrors} from 'app/actions/errors';
import {getHealth} from 'app/reducers/service';
import {getService, getErrors} from 'app/reducers';
import {getById} from 'app/reducers/errors';
import {timeIsOver} from '../lib/errors-logic';

export const START_CHECK_LOADING = 'START_CHECK_LOADING';
export const END_CHECK_LOADING = 'END_CHECK_LOADING';

export function setServiceHealtInProgress(serviceId, checkId, hours) {
    return dispatch => {
        return serviceApi.setServiceHealtInProgress(serviceId, checkId, hours).then(() => {
            return dispatch(fetchHealth(serviceId));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function fetchHealth(serviceId) {
    return (dispatch, getState) => {
        dispatch(startCheckLoading(serviceId));

        return serviceApi.fetchServiceHealth(serviceId).then(state => {
            const errors = getById(getErrors(getState()), 'dashboard');
            if (errors && errors.length) {
                dispatch(clearErrors('dashboard'));
            }

            return dispatch(endCheckLoading(serviceId, state.groups));
        }, error => {
            const lastUpdateTime = getHealth(getService(getState())).lastUpdateTimeHealth;

            if (timeIsOver(lastUpdateTime)) {
                return dispatch(setErrors('dashboard', [error.message]));
            }
        });
    };
}

export function startCheckLoading(serviceId) {
    return {
        type: START_CHECK_LOADING,
        serviceId
    };
}

export function endCheckLoading(serviceId, state) {
    return {
        type: END_CHECK_LOADING,
        serviceId,
        state,
        time: new Date()
    };
}
