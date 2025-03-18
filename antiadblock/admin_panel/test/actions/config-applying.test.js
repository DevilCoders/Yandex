import configureMockStore from 'redux-mock-store';
import thunk from 'redux-thunk';

import {STATUS} from 'app/enums/config';
import * as configApplyingActions from 'app/actions/config-applying';
import * as configApplyingActionsFixtures from '../fixtures/actions/config-applying';
import {CONFIG_ACTIVE, CONFIG_PREVIEW} from '../fixtures/backend/config';
import * as storeFixtures from '../fixtures/store';

const middlewares = [thunk];
const mockStore = configureMockStore(middlewares);

describe('config-applying actions', () => {
    it('should create OPEN_CONFIG_APPLYING', () => {
        expect(configApplyingActions.openConfigApplying(123, 236, CONFIG_PREVIEW, STATUS.ACTIVE)).toEqual(configApplyingActionsFixtures.OPEN_CONFIG_APPLYING);
    });

    it('should create CLOSE_CONFIG_APPLYING', () => {
        expect(configApplyingActions.closeConfigApplying(236)).toEqual(configApplyingActionsFixtures.CLOSE_CONFIG_APPLYING);
    });

    it('should create START_ACTIVE_CONFIG_LOADING', () => {
        expect(configApplyingActions.startActiveConfigLoading(123, STATUS.ACTIVE)).toEqual(configApplyingActionsFixtures.START_ACTIVE_CONFIG_LOADING);
    });

    it('should create END_ACTIVE_CONFIG_LOADING', () => {
        expect(configApplyingActions.endActiveConfigLoading(123, STATUS.ACTIVE, CONFIG_ACTIVE)).toEqual(configApplyingActionsFixtures.END_ACTIVE_CONFIG_LOADING);
    });

    it('should create START_ACTIVE_CONFIG_LOADING and END_ACTIVE_CONFIG_LOADING', () => {
        const store = mockStore(storeFixtures.CONFIG_APPLYING_OPENED);

        fetch.mockResponseOnce(JSON.stringify(CONFIG_ACTIVE));

        return store.dispatch(configApplyingActions.fetchCurrentConfig(123, STATUS.ACTIVE)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_ACTIVE_CONFIG_LOADING and SET_ERRORS', () => {
        const store = mockStore(storeFixtures.CONFIG_APPLYING_OPENED);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(configApplyingActions.fetchCurrentConfig(123, STATUS.ACTIVE)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_ACTIVE_CONFIG_LOADING and try to fetch ACTIVE config after error', () => {
        const store = mockStore(storeFixtures.CONFIG_APPLYING_OPENED);

        fetch.mockResponseOnce('Not found!', {status: 404});

        return store.dispatch(configApplyingActions.fetchCurrentConfig(123, STATUS.TEST)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_ACTIVE_CONFIG_LOADING and try to fetch ACTIVE config after error', () => {
        const store = mockStore(storeFixtures.CONFIG_APPLYING_OPENED);

        fetch.mockResponseOnce('Not found!', {status: 404});

        return store.dispatch(configApplyingActions.fetchCurrentConfig(123, STATUS.ACTIVE)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });
});
