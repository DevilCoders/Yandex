import * as experimentApplyingActions from 'app/actions/experiment-applying';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false
    };

    state = state || initialState;

    switch (type) {
        case experimentApplyingActions.OPEN_EXPERIMENT_APPLYING: {
            return {
                serviceId: action.serviceId,
                configId: action.configId,
                visible: true
            };
        }
        case experimentApplyingActions.CLOSE_EXPERIMENT_APPLYING: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
