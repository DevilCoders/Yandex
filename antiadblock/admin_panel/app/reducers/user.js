import * as userActions from 'app/actions/user';
import update from 'app/lib/update';

export default function(state, action) {
    const {type} = action;

    state = state || {
        username: null
    };

    switch (type) {
        case userActions.SET_USER_USERNAME: {
            return update(state, {
                $merge: {
                    username: action.value
                }
            });
        }
        default: {
            return state;
        }
    }
}
