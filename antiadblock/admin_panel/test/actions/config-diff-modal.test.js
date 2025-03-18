import * as configDiffModalActions from 'app/actions/config-diff-modal';
import * as configDiffModalActionsFixtures from '../fixtures/actions/config-diff-modal';

describe('config-diff-modal actions', () => {
    it('should create OPEN_CONFIG_DIFF', () => {
        expect(configDiffModalActions.openConfigDiffModal('yandex_afisha', 1, 3)).toEqual(configDiffModalActionsFixtures.OPEN_CONFIG_DIFF_MODAL);
    });

    it('should create CLOSE_CONFIG_DIFF', () => {
        expect(configDiffModalActions.closeConfigDiffModal()).toEqual(configDiffModalActionsFixtures.CLOSE_CONFIG_DIFF_MODAL);
    });
});
