import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        loading: true,
        loaded: false,
        data: {}
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING:
            return update(state, {
                $set: initialState
            });
        case serviceActions.CHANGE_LABEL_PARENT_START:
        case serviceActions.START_LABELS_LOADING: {
            return update(state, {
                $set: {
                    loading: true,
                    loaded: false,
                    data: state.data
                }
            });
        }
        case serviceActions.END_LABELS_LOADING: {
            return update(state, {
                $set: {
                    loaded: true,
                    loading: false,
                    data: action.labels
                }
            });
        }
        default: {
            return state;
        }
    }
}

