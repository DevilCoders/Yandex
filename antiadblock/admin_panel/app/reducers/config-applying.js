import * as configApplyingActions from 'app/actions/config-applying';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false,
        id: null,
        serviceId: null,
        target: null,
        configData: null,
        oldConfigData: null,
        loaded: false
    };

    state = state || initialState;

    switch (type) {
        case configApplyingActions.OPEN_CONFIG_APPLYING: {
            return {
                visible: true,
                id: action.id,
                serviceId: action.serviceId,
                target: action.target,
                configData: action.configData,
                oldConfigData: null,
                loaded: false
            };
        }
        case configApplyingActions.CLOSE_CONFIG_APPLYING: {
            return initialState;
        }
        case configApplyingActions.END_ACTIVE_CONFIG_LOADING: {
            return update(state, {
                $merge: {
                    oldConfigData: action.config,
                    loaded: true
                }
            });
        }
        default: {
            return state;
        }
    }
}
