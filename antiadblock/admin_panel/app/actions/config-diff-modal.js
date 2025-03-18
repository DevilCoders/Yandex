export const OPEN_CONFIG_DIFF_MODAL = 'OPEN_CONFIG_DIFF_MODAL';
export const CLOSE_CONFIG_DIFF_MODAL = 'CLOSE_CONFIG_DIFF_MODAL';

export function openConfigDiffModal(serviceId, id, secondId) {
    return {
        type: OPEN_CONFIG_DIFF_MODAL,
        serviceId,
        id,
        secondId
    };
}

export function closeConfigDiffModal() {
    return {
        type: CLOSE_CONFIG_DIFF_MODAL
    };
}
