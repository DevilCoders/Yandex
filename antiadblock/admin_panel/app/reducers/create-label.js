import * as labelsActions from 'app/actions/create-label';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false,
        serviceId: null,
        labelId: null
    };

    state = state || initialState;

    switch (type) {
        case labelsActions.OPEN_LABEL_CREATE_MODAL:
            return {
                visible: true,
                serviceId: action.serviceId,
                labelId: action.labelId
            };
        case labelsActions.CLOSE_LABEL_CREATE_MODAL:
            return initialState;
        default: {
            return state;
        }
    }
}
