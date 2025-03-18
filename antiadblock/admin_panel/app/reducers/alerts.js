import update from 'app/lib/update';

import * as alertsActions from 'app/actions/alerts';

export default function(state, action) {
    const {type} = action;

    state = state || {
        items: {},
        loaded: false
    };

    switch (type) {
        case alertsActions.START_ALERTS_LOADING: {
            return state;
        }
        case alertsActions.END_ALERTS_LOADING: {
            if (!action || typeof action.items !== 'object') {
                return state;
            }

            return update(state, {
               $merge: {
                    items: action.items,
                    loaded: true,
                    lastUpdateTimeAlerts: new Date()
               }
            });
        }
        default: {
            return state;
        }
    }
}
