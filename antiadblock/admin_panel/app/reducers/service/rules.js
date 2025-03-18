import * as serviceActions from 'app/actions/service/rules';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        loaded: false,
        data: {
            items: [],
            total: 0
        },
        schema: {}
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_RULES_LOADING:
            return update(state, {
                $set: initialState
            });
        case serviceActions.END_SERVICE_RULES_LOADING: {
            return update(state, {
                $merge: {
                    ...action.data,
                    loaded: true
                }
            });
        }
        default: {
            return state;
        }
    }
}
