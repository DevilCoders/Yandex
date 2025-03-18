import reducer from 'app/reducers/service/configs';
import * as serviceActionFixtures from '../../fixtures/actions/service';
import {CONFIGS} from '../../fixtures/store';

describe('configs', () => {
    const initialState = CONFIGS;

    describe('reducer', () => {
        it('should return the initial state', () => {
            expect(reducer(undefined, {})).toMatchSnapshot();
        });

        it('should handle START_SERVICE_CONFIG_ARCHIVED_SETTING', () => {
            expect(
                reducer(initialState, serviceActionFixtures.START_SERVICE_CONFIG_ARCHIVED_SETTING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_CONFIG_ARCHIVED_SETTING', () => {
            const afterConfigSettingStarted = reducer(initialState, serviceActionFixtures.START_SERVICE_CONFIG_ARCHIVED_SETTING);
            const afterConfigSetted = reducer(afterConfigSettingStarted, serviceActionFixtures.END_SERVICE_CONFIG_ARCHIVED_SETTING);

            expect(afterConfigSetted).toMatchSnapshot();
        });
    });
});
