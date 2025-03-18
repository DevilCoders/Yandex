import * as serviceActions from 'app/actions/service';
import * as metricActions from 'app/actions/metric';
import update from 'app/lib/update';

import {METRICS} from 'app/enums/graphs';

export default function(state, action) {
    const {type} = action;

    state = state || {};

    switch (type) {
        // При загрузке сервиса инициализируем state мониторинга пустыми объектами
        case serviceActions.END_SERVICE_LOADING: {
            return update(state, {
                $merge: {
                    [action.service.id]: {
                        ...Object.values(METRICS).reduce((acc, item) => {
                            acc[item] = {
                                loading: false,
                                loaded: false
                            };

                            return acc;
                        }, {})
                    }
                }
            });
        }
        case metricActions.START_METRIC_LOADING: {
            return update(state, {
                $merge: {
                    [action.serviceId]: {
                        ...state[action.serviceId],
                        [action.name]: {
                            range: action.range,
                            loaded: false,
                            loading: true
                        }
                    }
                }
            });
        }
        case metricActions.END_METRIC_LOADING: {
            if (state[action.serviceId][action.name].range === action.range) {
                return update(state, {
                    $merge: {
                        [action.serviceId]: {
                            ...state[action.serviceId],
                            [action.name]: {
                                data: action.data,
                                loading: false,
                                loaded: action.loaded
                            }
                        }
                    }
                });
            }

            return state;
        }
        default: {
            return state;
        }
    }
}

