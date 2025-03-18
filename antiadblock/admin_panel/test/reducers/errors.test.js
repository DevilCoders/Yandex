import reducer from 'app/reducers/errors';
import {getGlobal, getById} from 'app/reducers/errors';
import * as errorsActionFixtures from '../fixtures/actions/errors';
import {ERRORS} from '../fixtures/store';

describe('errors', () => {
    const initialState = ERRORS;

    describe('reducer', () => {
        it('should return the initial state', () => {
            expect(reducer(undefined, {})).toMatchSnapshot();
        });

        it('should handle SET_ERRORS', () => {
            expect(reducer(initialState, errorsActionFixtures.SET_ERRORS)).toMatchSnapshot();
        });

        it('should handle CLEAR_ERRORS', () => {
            expect(reducer(initialState, errorsActionFixtures.CLEAR_ERRORS)).toMatchSnapshot();
        });
    });

    describe('selectors', () => {
        it('getGlobal should return array with id global', () => {
            expect(getGlobal(initialState)).toMatchSnapshot();
        });

        it('getById should return array with id1', () => {
            expect(getById(initialState, 'id1')).toMatchSnapshot();
        });
    });
});
