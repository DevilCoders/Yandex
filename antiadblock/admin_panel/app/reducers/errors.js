import * as errorsActions from 'app/actions/errors';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;

    state = state || {
        byId: {}
    };

    switch (type) {
        case errorsActions.SET_ERRORS: {
            return update(state, {
                byId: {
                    $merge: {
                        [action.id]: action.errors
                    }
                }
            });
        }
        case errorsActions.CLEAR_ERRORS: {
            return update(state, {
                byId: {
                    $merge: {
                        [action.id]: []
                    }
                }
            });
        }
        default: {
            return state;
        }
    }
}

export function getGlobal(state) {
    const global = state.byId.global || [];
    return global;
}

export function getDashboard(state) {
    const dashboard = state.byId.dashboard || [];
    return dashboard;
}

export function getById(state, id) {
    return state.byId[id];
}
