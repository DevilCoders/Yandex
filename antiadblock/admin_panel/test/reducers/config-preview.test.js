import reducer from 'app/reducers/config-preview';
import * as configPreviewActionFixtures from '../fixtures/actions/config-preview';
import {CONFIG_PREVIEW_EMPTY} from '../fixtures/store';

describe('config-preview reducer', () => {
    const initialState = CONFIG_PREVIEW_EMPTY;

    it('should return the initial state', () => {
        expect(reducer(undefined, {})).toMatchSnapshot();
    });

    it('should handle OPEN_CONFIG_PREVIEW', () => {
        expect(
            reducer(initialState, configPreviewActionFixtures.OPEN_CONFIG_PREVIEW)
        ).toMatchSnapshot();
    });

    it('should handle CLOSE_CONFIG_PREVIEW', () => {
        const afterOpen = reducer(initialState, configPreviewActionFixtures.OPEN_CONFIG_PREVIEW);

        expect(
            reducer(afterOpen, configPreviewActionFixtures.CLOSE_CONFIG_PREVIEW)
        ).toMatchSnapshot();
    });
});
