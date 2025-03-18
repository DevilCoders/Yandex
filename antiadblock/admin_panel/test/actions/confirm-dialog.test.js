import * as confirmDialogActions from 'app/actions/confirm-dialog';
import * as confirmDialogActionsFixtures from '../fixtures/actions/confirm-dialog';

describe('confirm-dialog actions', () => {
    it('should create OPEN_CONFIRM_DIALOG', () => {
        const message = 'test message';
        const callback = 'test function';
        expect(confirmDialogActions.openConfirmDialog(message, callback)).toEqual(confirmDialogActionsFixtures.OPEN_CONFIRM_DIALOG);
    });

    it('should create CLOSE_CONFIRM_DIALOG', () => {
        expect(confirmDialogActions.closeConfirmDialog()).toEqual(confirmDialogActionsFixtures.CLOSE_CONFIRM_DIALOG);
    });
});
