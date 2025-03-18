import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        progress: false,
        id: null,
        approved: null,
        comment: null
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: { // TODO копипаста
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.START_SERVICE_CONFIG_MODERATING: {
            return update(state, {
                $merge: {
                    progress: true,
                    id: action.configId,
                    approved: action.approved,
                    comment: action.comment
                }
            });
        }
        case serviceActions.END_SERVICE_CONFIG_MODERATING: {
            return update(state, {
                $set: initialState
            });
        }
        default: {
            return state;
        }
    }
}
