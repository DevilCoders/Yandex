import * as confirmDialogActions from 'app/actions/confirm-dialog';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false,
        message: null,
        callback: null
    };

    state = state || initialState;

    switch (type) {
        case confirmDialogActions.OPEN_CONFIRM_DIALOG: {
            return {
                visible: true,
                message: action.message,
                callback: action.callback
            };
        }
        case confirmDialogActions.CLOSE_CONFIRM_DIALOG: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
