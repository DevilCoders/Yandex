export const OPEN_PROFILE_DIFF_MODAL = 'OPEN_PROFILE_DIFF_MODAL';
export const CLOSE_PROFILE_DIFF_MODAL = 'CLOSE_PROFILE_DIFF_MODAL';

export function openProfileDiffModal(serviceId, id, secondId) {
    return {
        type: OPEN_PROFILE_DIFF_MODAL,
        serviceId,
        id,
        secondId
    };
}

export function closeProfileDiffModal() {
    return {
        type: CLOSE_PROFILE_DIFF_MODAL
    };
}
