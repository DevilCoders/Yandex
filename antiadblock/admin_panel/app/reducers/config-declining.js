import * as configDecliningActions from 'app/actions/config-declining';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false,
        id: null,
        serviceId: null,
        labelId: null,
        approved: null
    };

    state = state || initialState;

    switch (type) {
        case configDecliningActions.OPEN_CONFIG_DECLINING: {
            return {
                visible: true,
                id: action.id,
                labelId: action.labelId,
                serviceId: action.serviceId,
                approved: action.approved
            };
        }
        case configDecliningActions.CLOSE_CONFIG_DECLINING: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
