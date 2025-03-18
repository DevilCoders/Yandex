import * as configPreviewActions from 'app/actions/config-preview';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false,
        id: null,
        serviceId: null,
        configData: null
    };

    state = state || initialState;

    switch (type) {
        case configPreviewActions.OPEN_CONFIG_PREVIEW: {
            return {
                visible: true,
                id: action.id,
                serviceId: action.serviceId,
                configData: action.configData
            };
        }
        case configPreviewActions.CLOSE_CONFIG_PREVIEW: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
