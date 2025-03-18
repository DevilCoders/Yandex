import {STATUS} from 'app/enums/config';
import * as configModeratingActions from 'app/actions/service';
import * as configModeratingActionsFixtures from '../fixtures/actions/config-moderating';
import {CONFIG_APPROVED, CONFIG_DECLINED} from '../fixtures/backend/config';

describe('config-approving actions', () => {
    it('should create START_SERVICE_CONFIG_MODERATING', () => {
        expect(configModeratingActions.startModerateServiceConfig(123, 239, STATUS.APPROVED, '')).toEqual(configModeratingActionsFixtures.START_SERVICE_CONFIG_APPROVING);
    });

    it('should create END_SERVICE_CONFIG_MODERATING', () => {
        expect(configModeratingActions.endModerateServiceConfig(123, 239, STATUS.APPROVED, '', CONFIG_APPROVED)).toEqual(configModeratingActionsFixtures.END_SERVICE_CONFIG_APPROVING);
    });

    it('should create START_SERVICE_CONFIG_MODERATING', () => {
        expect(configModeratingActions.startModerateServiceConfig(123, 239, STATUS.DECLINED, 'decline message')).toEqual(configModeratingActionsFixtures.START_SERVICE_CONFIG_DECLINING);
    });

    it('should create END_SERVICE_CONFIG_MODERATING', () => {
        expect(configModeratingActions.endModerateServiceConfig(123, 239, STATUS.DECLINED, 'decline message', CONFIG_DECLINED)).toEqual(configModeratingActionsFixtures.END_SERVICE_CONFIG_DECLINING);
    });
});
