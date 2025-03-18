export const OPEN_CONFIG_DECLINING = 'OPEN_CONFIG_DECLINING';
export const CLOSE_CONFIG_DECLINING = 'CLOSE_CONFIG_DECLINING';

export function openConfigDeclining(serviceId, labelId, id) {
    return {
        type: OPEN_CONFIG_DECLINING,
        serviceId,
        labelId,
        id
    };
}

export function closeConfigDeclining(id) {
    return {
        type: CLOSE_CONFIG_DECLINING,
        id
    };
}

