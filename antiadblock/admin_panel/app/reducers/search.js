import * as searchActions from 'app/actions/search';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;

    const initialState = {
        byId: {},
        items: [],
        loaded: false,
        total: 0
    };

    state = state || initialState;

    switch (type) {
        case searchActions.START_SEARCH: {
            return update(state, {
                loaded: {
                    $set: action.offset > 0
                },
                items: {
                    $set: action.offset > 0 ? state.items : []
                },
                pattern: {
                    $set: action.pattern
                },
                active: {
                    $set: action.active
                }
            });
        }
        case searchActions.END_SEARCH: {
            return update(state, {
                items: {
                    $push: action.configs.items.map(config => config.id)
                },
                byId: {
                    $merge: action.configs.items.reduce((map, config) => {
                        map[config.id] = config;

                        return map;
                    }, {})
                },
                pattern: {
                    $set: action.pattern
                },
                offset: {
                    $set: state.offset + action.configs.items.length
                },
                total: {
                    $set: action.configs.total
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
