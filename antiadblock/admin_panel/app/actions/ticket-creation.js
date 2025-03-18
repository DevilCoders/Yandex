export const OPEN_TICKET_CREATION = 'OPEN_TICKET_CREATION';
export const CLOSE_TICKET_CREATION = 'CLOSE_TICKET_CREATION';

export function openTicketCreation(serviceId) {
    return {
        type: OPEN_TICKET_CREATION,
        serviceId
    };
}

export function closeTicketCreation() {
    return {
        type: CLOSE_TICKET_CREATION
    };
}
