import * as searchActions from 'app/actions/search';
import * as searchActionsFixtures from '../fixtures/actions/search';

describe('search actions', () => {
    it('should create START_SEARCH', () => {
        expect(searchActions.startSearchConfigs('test', 0, 20, true)).toEqual(searchActionsFixtures.START_SEARCH);
    });

    it('should create END_SEARCH', () => {
        expect(searchActions.endSearchConfigs('test', {items: [], total: 0})).toEqual(searchActionsFixtures.END_SEARCH);
    });
});
