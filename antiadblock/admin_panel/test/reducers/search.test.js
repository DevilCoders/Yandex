import reducer from 'app/reducers/search';
import * as searchActionsFixtures from '../fixtures/actions/search';
import {BEFORE_SEARCH, START_SEARCH, END_SEARCH} from '../fixtures/store';

describe('search reducer', () => {
    it('should return the initial state', () => {
        expect(reducer(BEFORE_SEARCH, {})).toMatchSnapshot();
    });

    it('should handle START_SEARCH', () => {
        expect(
            reducer(START_SEARCH, searchActionsFixtures.START_SEARCH)
        ).toMatchSnapshot();
    });

    it('should handle END_SEARCH', () => {
        expect(
            reducer(END_SEARCH, searchActionsFixtures.END_SEARCH)
        ).toMatchSnapshot();
    });
});
