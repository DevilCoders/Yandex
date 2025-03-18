import * as serviceActions from 'app/actions/service';
import * as serviceActionsFixtures from '../fixtures/actions/service';
import configureMockStore from 'redux-mock-store';
import thunk from 'redux-thunk';
import * as storeFixtures from '../fixtures/store';
import {SERVICE, SERVICE_CONFIG, SERVICE_CONFIGS, SETUP, SERVICE_AUDIT, SERVICE_CONFIG_ARCHIVED} from '../fixtures/backend/service';

const middlewares = [thunk];
const mockStore = configureMockStore(middlewares);

describe('service actions', () => {
    it('should create START_SERVICE_LOADING', () => {
        expect(serviceActions.startServiceLoading(123)).toEqual(serviceActionsFixtures.START_SERVICE_LOADING);
    });

    it('should create END_SERVICE_LOADING', () => {
        expect(serviceActions.endServiceLoading(123, SERVICE)).toEqual(serviceActionsFixtures.END_SERVICE_LOADING);
    });

    it('should create START_SERVICE_LOADING and END_SERVICE_CONFIG_LOADING', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockResponseOnce(JSON.stringify(SERVICE));

        return store.dispatch(serviceActions.fetchService(123)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_LOADING and SET_ERRORS', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(serviceActions.fetchService(123)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIGS_LOADING', () => {
        expect(serviceActions.startServiceConfigsLoading(123, 0, 20)).toEqual(serviceActionsFixtures.START_SERVICE_CONFIGS_LOADING);
    });

    it('should create END_SERVICE_CONFIGS_LOADING', () => {
        expect(serviceActions.endServiceConfigsLoading(123, SERVICE_CONFIGS)).toEqual(serviceActionsFixtures.END_SERVICE_CONFIGS_LOADING);
    });

    it('should create START_SERVICE_CONFIGS_LOADING and END_SERVICE_CONFIGS_LOADING', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockResponseOnce(JSON.stringify(SERVICE_CONFIGS));

        return store.dispatch(serviceActions.fetchServiceConfigs(123, 0, 20)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIGS_LOADING and SET_ERRORS', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(serviceActions.fetchServiceConfigs(123, 0, 20)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIG_LOADING', () => {
        expect(serviceActions.startServiceConfigLoading(123, 456)).toEqual(serviceActionsFixtures.START_SERVICE_CONFIG_LOADING);
    });

    it('should create END_SERVICE_CONFIG_LOADING', () => {
        expect(serviceActions.endServiceConfigLoading(123, 456, SERVICE_CONFIG)).toEqual(serviceActionsFixtures.END_SERVICE_CONFIG_LOADING);
    });

    it('should create START_SERVICE_CONFIG_LOADING and END_SERVICE_CONFIG_LOADING', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockResponseOnce(JSON.stringify(SERVICE_CONFIG));

        return store.dispatch(serviceActions.fetchConfig(123, 456)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_LOADING and SET_ERRORS', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(serviceActions.fetchConfig(123, 456)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_LOADING and END_SERVICE_CONFIG_LOADING with empty data', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockResponseOnce('Not found', {status: 404});

        return store.dispatch(serviceActions.fetchConfig(123, 456)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIG_APPLYING with target=active', () => {
        expect(serviceActions.startServiceConfigApplying(123, 234, {
            target: 'active',
            oldConfigId: 235
        })).toEqual(serviceActionsFixtures.START_SERVICE_CONFIG_APPLYING);
    });

    it('should create END_SERVICE_CONFIG_APPLYING with target=active', () => {
        expect(serviceActions.endServiceConfigApplying(123, 234, {
            target: 'active',
            oldConfigId: 235
        })).toEqual(serviceActionsFixtures.END_SERVICE_CONFIG_APPLYING);
    });

    it('should create START_SERVICE_CONFIG_APPLYING and END_SERVICE_CONFIG_APPLYING with target=active', () => {
        const store = mockStore(storeFixtures.SERVICE_WITH_CONFIGS);

        fetch.mockResponseOnce(JSON.stringify({}));

        return store.dispatch(serviceActions.applyConfig(123, 234, {
            target: 'active',
            oldConfigId: 235
        })).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIG_APPLYING and SET_ERRORS with target=active', () => {
        const store = mockStore(storeFixtures.SERVICE_WITH_CONFIGS);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(serviceActions.applyConfig(123, 234, {
            target: 'active',
            oldConfigId: 235
        })).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_SETUP_LOADING', () => {
        expect(serviceActions.startServiceSetupLoading(123)).toEqual(serviceActionsFixtures.START_SERVICE_SETUP_LOADING);
    });

    it('should create END_SERVICE_SETUP_LOADING', () => {
        expect(serviceActions.endServiceSetupLoading(123, SETUP)).toEqual(serviceActionsFixtures.END_SERVICE_SETUP_LOADING);
    });

    it('should create START_SERVICE_SETUP_LOADING and END_SERVICE_SETUP_LOADING', () => {
        const store = mockStore(storeFixtures.SERVICE);

        // fetch.mockResponseOnce(JSON.stringify(SETUP));

        return store.dispatch(serviceActions.fetchServiceSetup(123)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_AUDIT_LOADING', () => {
        expect(serviceActions.startServiceAuditLoading('yandex_afisha', 0, 20)).toEqual(serviceActionsFixtures.START_SERVICE_AUDIT_LOADING);
    });

    it('should create START_SERVICE_AUDIT_LOADING with default params', () => {
        expect(serviceActions.resetServiceAudit('yandex_afisha')).toEqual(serviceActionsFixtures.RESET_SERVICE_AUDIT);
    });

    it('should create END_SERVICE_AUDIT_LOADING', () => {
        expect(serviceActions.endServiceAuditLoading('yandex_afisha', SERVICE_AUDIT)).toEqual(serviceActionsFixtures.END_SERVICE_AUDIT_LOADING);
    });

    it('should create START_SERVICE_AUDIT_LOADING and END_SERVICE_AUDIT_LOADING', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockResponseOnce(JSON.stringify(SERVICE_AUDIT));

        return store.dispatch(serviceActions.fetchServiceAudit('yandex_afisha', 0, 20)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_AUDIT_LOADING and SET_ERRORS', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(serviceActions.fetchServiceAudit('yandex_afisha', 0, 20)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIG_ARCHIVED_SETTING', () => {
        expect(serviceActions.startServiceConfigArchivedSetting('yandex_afisha', 345, true)).toEqual(serviceActionsFixtures.START_SERVICE_CONFIG_ARCHIVED_SETTING);
    });

    it('should create END_SERVICE_CONFIG_ARCHIVED_SETTING', () => {
        expect(serviceActions.endServiceConfigArchivedSetting('yandex_afisha', 345, true)).toEqual(serviceActionsFixtures.END_SERVICE_CONFIG_ARCHIVED_SETTING);
    });

    it('should create START_SERVICE_CONFIG_ARCHIVED_SETTING and END_SERVICE_CONFIG_ARCHIVED_SETTING', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockResponseOnce(JSON.stringify(SERVICE_CONFIG_ARCHIVED));

        return store.dispatch(serviceActions.setArchivedConfig('yandex_afisha', 345, true)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_CONFIG_ARCHIVED_SETTING and SET_ERRORS', () => {
        const store = mockStore(storeFixtures.SERVICE_EMPTY);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(serviceActions.setArchivedConfig('yandex_afisha', 345, true)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_SERVICE_COMMENT', () => {
        expect(serviceActions.startServiceComment(123)).toEqual(serviceActionsFixtures.START_SERVICE_COMMENT);
    });

    it('should create END_SERVICE_COMMENT', () => {
        expect(serviceActions.endServiceComment(123, 'comment')).toEqual(serviceActionsFixtures.END_SERVICE_COMMENT);
    });
});
