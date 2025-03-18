import {SERVICES} from '../backend/services';

export const START_SERVICES_LOADING = {
    type: 'START_SERVICES_LOADING'
};

export const END_SERVICES_LOADING = {
    type: 'END_SERVICES_LOADING',
    items: SERVICES.items
};
