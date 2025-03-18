import * as serviceActions from 'app/actions/service';
import * as schemaActions from 'app/actions/schema';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        schema: [],
        loaded: false
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING:
        case schemaActions.START_SCHEMA_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case schemaActions.END_SCHEMA_LOADING: {
            return update(state, {
                $set: {
                    schema: [...action.schema],
                    loaded: true
                }
            });
        }
        default: {
            return state;
        }
    }
}

