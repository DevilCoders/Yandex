import reducer from 'app/reducers/config-diff-modal';
import * as configDiffModalActionFixtures from '../fixtures/actions/config-diff-modal';
import {CONFIG_DIFF_EMPTY, CONFIG_DIFF_OPENED} from '../fixtures/store';

describe('config-diff-modal reducer', () => {
    const initialState = CONFIG_DIFF_EMPTY;
    const openedState = CONFIG_DIFF_OPENED;

    it('should return the initial state', () => {
        expect(reducer(undefined, {})).toMatchSnapshot();
    });

    it('should handle OPEN_CONFIG_DIFF', () => {
        expect(
            reducer(initialState, configDiffModalActionFixtures.OPEN_CONFIG_DIFF_MODAL)
        ).toMatchSnapshot();
    });

    it('should handle CLOSE_CONFIG_DIFF', () => {
        expect(
            reducer(openedState, configDiffModalActionFixtures.CLOSE_CONFIG_DIFF_MODAL)
        ).toMatchSnapshot();
    });
});
