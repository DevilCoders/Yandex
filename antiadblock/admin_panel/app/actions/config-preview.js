export const OPEN_CONFIG_PREVIEW = 'OPEN_CONFIG_PREVIEW';
export const CLOSE_CONFIG_PREVIEW = 'CLOSE_CONFIG_PREVIEW';

export function openConfigPreview(serviceId, id, configData) {
    return {
        type: OPEN_CONFIG_PREVIEW,
        serviceId,
        id,
        configData
    };
}

export function closeConfigPreview(id) {
    return {
        type: CLOSE_CONFIG_PREVIEW,
        id
    };
}
