import reducer from 'app/reducers/service';
import {getConfigs, getConfigById, getAudit, getAuditItemById, getSchema} from 'app/reducers/service';
import * as serviceActionFixtures from '../fixtures/actions/service';
import {SERVICE_EMPTY, SERVICE_WITH_CONFIGS} from '../fixtures/store';

describe('service', () => {
    const initialState = SERVICE_EMPTY;

    describe('reducer', () => {
        let afterServiceLoaded,
            afterServiceConfigsLoaded;

        it('should return the initial state', () => {
            expect(reducer(undefined, {})).toMatchSnapshot();
        });

        it('should handle START_SERVICE_LOADING', () => {
            expect(
                reducer(initialState, serviceActionFixtures.START_SERVICE_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_LOADING', () => {
            const afterServiceLoadingStarted = reducer(initialState, serviceActionFixtures.START_SERVICE_LOADING);
            afterServiceLoaded = reducer(afterServiceLoadingStarted, serviceActionFixtures.END_SERVICE_LOADING);

            expect(afterServiceLoaded).toMatchSnapshot();
        });

        it('should handle START_SERVICE_CONFIG_LOADING', () => {
            expect(
                reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_CONFIG_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_CONFIG_LOADING', () => {
            const afterServiceConfigLoadingStarted = reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_CONFIG_LOADING);
            afterServiceConfigsLoaded = reducer(afterServiceConfigLoadingStarted, serviceActionFixtures.END_SERVICE_CONFIG_LOADING);

            expect(afterServiceConfigsLoaded).toMatchSnapshot();
        });

        it('should handle START_SERVICE_CONFIGS_LOADING', () => {
            expect(
                reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_CONFIGS_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_CONFIGS_LOADING', () => {
            const afterServiceConfigsLoadingStarted = reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_CONFIGS_LOADING);
            afterServiceConfigsLoaded = reducer(afterServiceConfigsLoadingStarted, serviceActionFixtures.END_SERVICE_CONFIGS_LOADING);

            expect(afterServiceConfigsLoaded).toMatchSnapshot();
        });

        it('should handle START_SERVICE_CONFIG_APPLYING', () => {
            expect(
                reducer(afterServiceConfigsLoaded, serviceActionFixtures.START_SERVICE_CONFIG_APPLYING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_CONFIG_APPLYING', () => {
            const afterServiceConfigApplyingStarted = reducer(afterServiceConfigsLoaded, serviceActionFixtures.START_SERVICE_CONFIG_APPLYING);

            expect(
                reducer(afterServiceConfigApplyingStarted, serviceActionFixtures.END_SERVICE_CONFIG_APPLYING)
            ).toMatchSnapshot();
        });

        it('should handle START_SERVICE_SETUP_LOADING', () => {
            expect(
                reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_SETUP_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_SETUP_LOADING', () => {
            const afterServiceSetupLoadingStarted = reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_SETUP_LOADING);

            expect(
                reducer(afterServiceSetupLoadingStarted, serviceActionFixtures.END_SERVICE_SETUP_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle START_SERVICE_AUDIT_LOADING', () => {
            expect(
                reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_AUDIT_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_AUDIT_LOADING', () => {
            const afterServiceAuditLoadingStarted = reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_AUDIT_LOADING);
            const afterServiceAuditLoaded = reducer(afterServiceAuditLoadingStarted, serviceActionFixtures.END_SERVICE_AUDIT_LOADING);

            expect(afterServiceAuditLoaded).toMatchSnapshot();
        });

        it('should handle RESET_SERVICE_AUDIT', () => {
            const afterServiceAuditLoadingStarted = reducer(afterServiceLoaded, serviceActionFixtures.START_SERVICE_AUDIT_LOADING);
            const afterServiceAuditLoaded = reducer(afterServiceAuditLoadingStarted, serviceActionFixtures.END_SERVICE_AUDIT_LOADING);

            expect(
                reducer(afterServiceAuditLoaded, serviceActionFixtures.RESET_SERVICE_AUDIT)
            ).toMatchSnapshot();
        });

        it('should handle END_SERVICE_COMMENT', () => {
            const afterServiceComment = reducer(afterServiceLoaded, serviceActionFixtures.END_SERVICE_COMMENT);

            expect(afterServiceComment).toMatchSnapshot();
        });
    });

    describe('selectors', () => {
        test('getConfigs should return configs from state', () => {
            expect(getConfigs(SERVICE_WITH_CONFIGS)).toMatchSnapshot();
        });

        test('getConfigById should return config with id=235', () => {
            expect(getConfigById(SERVICE_WITH_CONFIGS, 235)).toMatchSnapshot();
        });

        test('getAudit should return audit from state', () => {
            expect(getAudit(SERVICE_WITH_CONFIGS)).toMatchSnapshot();
        });

        test('getAuditItemById should return audit with id=3', () => {
            expect(getAuditItemById(SERVICE_WITH_CONFIGS, 3)).toMatchSnapshot();
        });

        test('getSchema should return schema', () => {
            expect(getSchema(SERVICE_WITH_CONFIGS)).toMatchSnapshot();
        });
    });
});
