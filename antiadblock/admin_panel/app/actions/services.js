import servicesApi from 'app/api/services';
import {setGlobalErrors} from 'app/actions/errors';

export const START_SERVICES_LOADING = 'START_SERVICES_LOADING';
export const END_SERVICES_LOADING = 'END_SERVICES_LOADING';
export const START_ALERTS_LOADING = 'START_ALERTS_LOADING';
export const END_ALERTS_LOADING = 'END_ALERTS_LOADING';

export function fetchServices() {
    return dispatch => {
        dispatch(startServicesLoading());

        return servicesApi.fetchServices().then(services => {
            return dispatch(endServicesLoading(services.items));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startServicesLoading() {
    return {
        type: START_SERVICES_LOADING
    };
}

export function endServicesLoading(items) {
    return {
        type: END_SERVICES_LOADING,
        items
    };
}
