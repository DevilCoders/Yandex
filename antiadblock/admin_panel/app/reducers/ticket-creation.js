import * as ticketCreationActions from 'app/actions/ticket-creation';

export default function(state, action) {
    const {type} = action;
    const initialState = {
        visible: false
    };

    state = state || initialState;

    switch (type) {
        case ticketCreationActions.OPEN_TICKET_CREATION: {
            return {
                serviceId: action.serviceId,
                visible: true
            };
        }
        case ticketCreationActions.CLOSE_TICKET_CREATION: {
            return initialState;
        }
        default: {
            return state;
        }
    }
}
