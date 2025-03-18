import * as configActions from 'app/actions/config';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        dataCurrent: {},
        dataParent: {},
        dataSettings: {},
        loaded: false,
        labelId: '',
        comment: '',
        id: 0,
        validation: {
            comment: [],
            data: []
        },
        saving: false,
        status: ''
    };

    state = state || initialState;

    switch (type) {
        case configActions.START_FETCH_CONFIG_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case configActions.END_FETCH_CONFIG_LOADING: {
            return update(state, {
                $merge: {
                    id: action.data.id,
                    dataParent: action.data.parent_data,
                    dataCurrent: action.data.dataCurrent,
                    dataSettings: action.data.data_settings,
                    loaded: true,
                    labelId: action.data.data_settings,
                    comment: action.data.comment,
                    status: action.data.status
                }
            });
        }
        case configActions.START_CONFIG_SAVING: {
            return update(state, {
               $merge: {
                   saving: true
               }
            });
        }
        case configActions.END_CONFIG_SAVING: {
            const newState = (action.validation.comment.length || Object.keys(action.validation.data).length) ? {
                saving: false,
                validation: action.validation || initialState.validation
            } : initialState;

            return update(state, {
               $merge: {
                 ...newState
               }
            });
        }
        default: {
            return state;
        }
    }
}
