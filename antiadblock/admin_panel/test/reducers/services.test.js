import reducer from 'app/reducers/services';
import * as servicesActionFixtures from '../fixtures/actions/services';
import {SERVICES_EMPTY} from '../fixtures/store';

describe('services', () => {
    const initialState = SERVICES_EMPTY;

    describe('reducer', () => {
        it('should return the initial state', () => {
            expect(reducer(undefined, {})).toMatchSnapshot();
        });

        it('should handle START_SERVICES_LOADING', () => {
            expect(
                reducer(initialState, servicesActionFixtures.START_SERVICES_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICES_LOADING', () => {
            expect(
                reducer(initialState, servicesActionFixtures.END_SERVICES_LOADING)
            ).toMatchSnapshot();
        });
    });
});
