import * as configPreviewActions from 'app/actions/config-preview';
import * as configPreviewActionsFixtures from '../fixtures/actions/config-preview';
import {CONFIG_ACTIVE} from '../fixtures/backend/config';

describe('config-preview actions', () => {
    it('should create OPEN_CONFIG_PREVIEW', () => {
        expect(configPreviewActions.openConfigPreview(123, 234, CONFIG_ACTIVE)).toEqual(configPreviewActionsFixtures.OPEN_CONFIG_PREVIEW);
    });

    it('should create CLOSE_CONFIG_PREVIEW', () => {
        expect(configPreviewActions.closeConfigPreview(234)).toEqual(configPreviewActionsFixtures.CLOSE_CONFIG_PREVIEW);
    });
});
