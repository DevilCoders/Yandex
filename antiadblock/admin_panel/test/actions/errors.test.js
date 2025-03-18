import * as errorsActions from 'app/actions/errors';
import * as errorsActionsFixtures from '../fixtures/actions/errors';

describe('errors actions', () => {
    const id = 'global';
    const errors = [
        'error message 1',
        'error message 2'
    ];

    it('should create SET_ERRORS', () => {
        expect(errorsActions.setErrors(id, errors)).toEqual(errorsActionsFixtures.SET_ERRORS);
    });

    it('should create CLEAR_ERRORS', () => {
        expect(errorsActions.clearErrors(id)).toEqual(errorsActionsFixtures.CLEAR_ERRORS);
    });
});
