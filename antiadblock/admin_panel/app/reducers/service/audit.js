import * as serviceActions from 'app/actions/service';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        byId: {},
        items: [],
        loaded: false,
        offset: 0,
        total: 0
    };

    state = state || initialState;

    switch (type) {
        case serviceActions.START_SERVICE_LOADING: { // TODO копипаста
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.RESET_SERVICE_AUDIT: {
            return update(state, {
                $set: initialState
            });
        }
        case serviceActions.START_SERVICE_AUDIT_LOADING: {
            return update(state, {
                loaded: {
                    $set: action.offset > 0
                }
            });
        }
        case serviceActions.END_SERVICE_AUDIT_LOADING: {
            return update(state, {
                items: {
                    $push: action.audit.items.map(auditItem => auditItem.id)
                },
                byId: {
                    $merge: action.audit.items.reduce((map, auditItem) => {
                        map[auditItem.id] = auditItem;

                        return map;
                    }, {})
                },
                offset: {
                    $set: state.offset + action.audit.items.length
                },
                total: {
                    $set: action.audit.total
                },
                loaded: {
                    $set: true
                }
            });
        }
        default: {
            return state;
        }
    }
}
