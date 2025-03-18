import * as serviceActions from 'app/actions/service';
import * as healthActions from 'app/actions/health';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        loading: true,
        loaded: false,
        state: [],
        lastUpdateTimeHealth: 0
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING:
            return update(state, {
                $set: initialState
            });
        case healthActions.START_CHECK_LOADING: {
            return update(state, {
                $set: {
                    loading: true,
                    loaded: false,
                    state: state.state
                }
            });
        }
        case healthActions.END_CHECK_LOADING: {
            return update(state, {
                $set: {
                    loaded: true,
                    loading: false,
                    state: action.state,
                    lastUpdateTimeHealth: action.time
                }
            });
        }
        default: {
            return state;
        }
    }
}

