import * as configDiffModalActions from 'app/actions/config-diff-modal';

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
        case configDiffModalActions.OPEN_CONFIG_DIFF_MODAL: {
            return {
                id: action.id,
                secondId: action.secondId,
                serviceId: action.serviceId,
                modalVisible: true
            };
        }
        case configDiffModalActions.CLOSE_CONFIG_DIFF_MODAL: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
