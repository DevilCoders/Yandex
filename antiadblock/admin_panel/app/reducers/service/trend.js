import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        loaded: false,
        creating: false,
        ticket_id: null
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_TREND_LOADING: {
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.END_SERVICE_TREND_LOADING: {
            return update(state, {
                $merge: {
                    ticket_id: action.trend.ticket_id,
                    loaded: action.loaded,
                    creating: false
                }
            });
        }
        case serviceActions.START_SERVICE_TREND_CREATION: {
            return update(state, {
                $merge: {
                    loaded: true,
                    creating: true,
                    ticket_id: null
                }
            });
        }
        case serviceActions.END_SERVICE_TREND_CREATION: {
            return update(state, {
                $merge: {
                    loaded: true,
                    creating: false,
                    ticket_id: action.trend.ticket_id
                }
            });
        }
        default: {
            return state;
        }
    }
}
