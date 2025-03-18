import * as profileDiffModalActions from 'app/actions/profile-diff-modal';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        id: null,
        secondId: null,
        serviceId: null,
        modalVisible: false
    };

    state = state || initialState;

    switch (type) {
        case profileDiffModalActions.OPEN_PROFILE_DIFF_MODAL: {
            return {
                id: action.id,
                secondId: action.secondId,
                serviceId: action.serviceId,
                modalVisible: true
            };
        }
        case profileDiffModalActions.CLOSE_PROFILE_DIFF_MODAL: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
