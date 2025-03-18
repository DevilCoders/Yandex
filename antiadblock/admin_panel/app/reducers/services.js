import * as servicesActions from 'app/actions/services';
import update from 'app/lib/update';
import * as serviceActions from 'app/actions/service';
import {STATUS} from 'app/enums/service';

export default function(state, action) {
    const {type} = action;

    state = state || {
        items: [],
        loading: false,
        loaded: false
    };

    switch (type) {
        case servicesActions.START_SERVICES_LOADING: {
            return update(state, {
                loading: {
                    $set: true
                }
            });
        }
        case servicesActions.END_SERVICES_LOADING: {
            return update(state, {
                $merge: {
                    items: action.items,
                    loading: false,
                    loaded: true
                }
            });
        }
        case serviceActions.END_SERVICE_DISABLING: {
            return update(state, {
                $byId: {
                    path: 'items',
                    key: 'id',
                    id: action.serviceId,
                    fallback: {},
                    do: {
                        $merge: {
                            status: STATUS.INACTIVE
                        }
                    }
                }
            });
        }
        case serviceActions.END_SERVICE_ENABLING: {
            return update(state, {
                $byId: {
                    path: 'items',
                    key: 'id',
                    id: action.serviceId,
                    fallback: {},
                    do: {
                        $merge: {
                            status: STATUS.ACTIVE
                        }
                    }
                }
            });
        }
        default: {
            return state;
        }
    }
}
