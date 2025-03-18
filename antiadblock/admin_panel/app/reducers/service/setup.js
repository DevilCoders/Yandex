import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        loaded: false,
        data: null
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: { // TODO копипаста
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.START_SERVICE_SETUP_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.END_SERVICE_SETUP_LOADING: {
            return update(state, {
                $merge: {
                    data: action.setup,
                    loaded: true
                }
            });
        }
        default: {
            return state;
        }
    }
}
