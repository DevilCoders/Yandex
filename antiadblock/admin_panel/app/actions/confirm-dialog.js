export const OPEN_CONFIRM_DIALOG = 'OPEN_CONFIRM_DIALOG';
export const CLOSE_CONFIRM_DIALOG = 'CLOSE_CONFIRM_DIALOG';

export function openConfirmDialog(message, callback) {
    return {
        type: OPEN_CONFIRM_DIALOG,
        message,
        callback
    };
}

export function closeConfirmDialog() {
    return {
        type: CLOSE_CONFIRM_DIALOG
    };
}
