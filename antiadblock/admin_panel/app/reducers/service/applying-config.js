import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        progress: false,
        id: null,
        options: null // target: [preview, active]; oldConfigId: number?
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: { // TODO копипаста
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.START_SERVICE_CONFIG_APPLYING: {
            return update(state, {
                $merge: {
                    progress: true,
                    id: action.configId,
                    options: action.options
                }
            });
        }
        case serviceActions.END_SERVICE_CONFIG_APPLYING: {
            return update(state, {
                $set: initialState
            });
        }
        default: {
            return state;
        }
    }
}
