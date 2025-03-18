import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';
import {STATUS} from '../../enums/service';

export default function(state, action) {
    const {type} = action;
    const initialState = null;

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.END_SERVICE_LOADING: {
            return update(state, {
                $set: {
                    ...action.service,
                    optionalMonitorings: {
                        mobile_monitorings_enabled: action.service.mobile_monitorings_enabled
                    },
                    settingEnableMonitoring: false,
                    settingSupportPriority: false
                }
            });
        }
        case serviceActions.END_SERVICE_DISABLING: {
            return action.serviceId === state.id ? update(state, {
                $merge: {
                    status: STATUS.INACTIVE
                }
            }) : state;
        }
        case serviceActions.END_SERVICE_ENABLING: {
            return action.serviceId === state.id ? update(state, {
                $merge: {
                    status: STATUS.ACTIVE
                }
            }) : state;
        }
        case serviceActions.START_SET_SERVICE_ENABLE_MONITORING: {
            return action.serviceId === state.id ? update(state, {
                $merge: {
                    settingEnableMonitoring: true
                }
            }) : state;
        }
        case serviceActions.END_SET_SERVICE_ENABLE_MONITORING: {
            return action.serviceId === state.id ? update(state, {
                $merge: action.monitoringName === 'monitorings' ? {
                    settingEnableMonitoring: false,
                    monitorings_enabled: action.monitoringIsEnable,
                    optionalMonitorings: Object.keys(state.optionalMonitorings).reduce((acc, key) => {
                        acc[key] = action.monitoringIsEnable;
                        return acc;
                    }, {})
                } : {
                    settingEnableMonitoring: false,
                    optionalMonitorings: {
                        ...state.optionalMonitorings,
                        [`${action.monitoringName}_enabled`]: action.monitoringIsEnable
                    }
                }
            }) : state;
        }
        case serviceActions.START_SERVICE_CHANGE_PRIORITY: {
            return action.serviceId === state.id ? update(state, {
                $merge: {
                    settingSupportPriority: true
                }
            }) : state;
        }
        case serviceActions.END_SERVICE_CHANGE_PRIORITY: {
            return action.serviceId === state.id ? update(state, {
                $merge: {
                    settingSupportPriority: false,
                    support_priority: action.priority
                }
            }) : state;
        }
        default: {
            return state;
        }
    }
}
