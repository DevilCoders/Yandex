import reducer from 'app/reducers/confirm-dialog';
import * as confirmDialogActionFixtures from '../fixtures/actions/confirm-dialog';
import {CONFIRM_DIALOG_EMPTY} from '../fixtures/store';

describe('confirm-dialog reducer', () => {
    const initialState = CONFIRM_DIALOG_EMPTY;

    it('should return the initial state', () => {
        expect(reducer(undefined, {})).toMatchSnapshot();
    });

    it('should handle OPEN_CONFIRM_DIALOG', () => {
        expect(
            reducer(initialState, confirmDialogActionFixtures.OPEN_CONFIRM_DIALOG)
        ).toMatchSnapshot();
    });

    it('should handle CLOSE_CONFIRM_DIALOG', () => {
        const afterOpen = reducer(initialState, confirmDialogActionFixtures.OPEN_CONFIRM_DIALOG);

        expect(
            reducer(afterOpen, confirmDialogActionFixtures.CLOSE_CONFIRM_DIALOG)
        ).toMatchSnapshot();
    });
});
