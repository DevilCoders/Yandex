import reducer from 'app/reducers/config-applying';
import * as configApplyingActionFixtures from '../fixtures/actions/config-applying';
import {CONFIG_APPLYING_EMPTY, CONFIG_APPLYING_OPENED, CONFIG_APPLYING_LOADED} from '../fixtures/store';

describe('config-applying reducer', () => {
    const initialState = CONFIG_APPLYING_EMPTY;
    const openedState = CONFIG_APPLYING_OPENED;
    const loadedState = CONFIG_APPLYING_LOADED;

    it('should return the initial state', () => {
        expect(reducer(undefined, {})).toMatchSnapshot();
    });

    it('should handle OPEN_CONFIG_APPLYING', () => {
        expect(
            reducer(initialState, configApplyingActionFixtures.OPEN_CONFIG_APPLYING)
        ).toMatchSnapshot();
    });

    it('should handle START_ACTIVE_CONFIG_LOADING', () => {
        expect(
            reducer(openedState, configApplyingActionFixtures.START_ACTIVE_CONFIG_LOADING)
        ).toMatchSnapshot();
    });

    it('should handle END_ACTIVE_CONFIG_LOADING', () => {
        expect(
            reducer(loadedState, configApplyingActionFixtures.END_ACTIVE_CONFIG_LOADING)
        ).toMatchSnapshot();
    });

    it('should handle CLOSE_CONFIG_PREVIEW', () => {
        expect(
            reducer(loadedState, configApplyingActionFixtures.CLOSE_CONFIG_APPLYING)
        ).toMatchSnapshot();
    });
});
