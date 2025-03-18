import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        progress: false
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.START_SERVICE_EXPERIMENT_APPLYING:
        case serviceActions.START_CONFIG_REMOVE_EXPERIMENT: {
            return update(state, {
                $merge: {
                    progress: true
                }
            });
        }
        case serviceActions.END_SERVICE_EXPERIMENT_APPLYING:
        case serviceActions.END_CONFIG_REMOVE_EXPERIMENT: {
            return update(state, {
                $set: initialState
            });
        }
        default: {
            return state;
        }
    }
}
