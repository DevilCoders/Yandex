import servicesApi from 'app/api/services';
import {setErrors, clearErrors} from 'app/actions/errors';
import {getServices, getErrors} from 'app/reducers/index';
import {getById} from 'app/reducers/errors';
import {timeIsOver} from 'app/lib/errors-logic';

export const START_ALERTS_LOADING = 'START_ALERTS_LOADING';
export const END_ALERTS_LOADING = 'END_ALERTS_LOADING';

export function fetchAlerts() {
    return (dispatch, getState) => {
        dispatch(startAlertsLoading());

        return servicesApi.fetchAlerts().then(alerts => {
            const errors = getById(getErrors(getState()), 'dashboard');
            if (errors && errors.length) {
                dispatch(clearErrors('dashboard'));
            }

            return dispatch(endAlertsLoading(alerts.items));
        }, error => {
            const lastUpdateTime = getServices(getState()).lastUpdateTimeAlerts;

            if (timeIsOver(lastUpdateTime)) {
                return dispatch(setErrors('dashboard', [error.message]));
            }
        });
    };
}

export function startAlertsLoading() {
    return {
        type: START_ALERTS_LOADING
    };
}

export function endAlertsLoading(items) {
    return {
        type: END_ALERTS_LOADING,
        items
    };
}
