export const OPEN_LABEL_CREATE_MODAL = 'OPEN_LABEL_CREATE_MODAL';
export const CLOSE_LABEL_CREATE_MODAL = 'CLOSE_LABEL_CREATE_MODAL';

export function openLabelCreateModal(serviceId, labelId) {
    return {
        type: OPEN_LABEL_CREATE_MODAL,
        serviceId,
        labelId
    };
}

export function closeLabelCreateModal() {
    return {
        type: CLOSE_LABEL_CREATE_MODAL
    };
}
