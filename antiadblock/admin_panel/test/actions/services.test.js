import * as servicesActions from 'app/actions/services';
import * as servicesActionsFixtures from '../fixtures/actions/services';
import configureMockStore from 'redux-mock-store';
import thunk from 'redux-thunk';
import * as storeFixtures from '../fixtures/store';
import {SERVICES} from '../fixtures/backend/services';

const middlewares = [thunk];
const mockStore = configureMockStore(middlewares);

describe('services actions', () => {
    it('should create START_SERVICES_LOADING', () => {
        expect(servicesActions.startServicesLoading()).toEqual(servicesActionsFixtures.START_SERVICES_LOADING);
    });

    it('should create END_SERVICES_LOADING', () => {
        expect(servicesActions.endServicesLoading(SERVICES.items)).toEqual(servicesActionsFixtures.END_SERVICES_LOADING);
    });

    it('should create END_SERVICES_LOADING after loading', () => {
        const store = mockStore(storeFixtures.SERVICES_EMPTY);

        fetch.mockResponseOnce(JSON.stringify(SERVICES));

        return store.dispatch(servicesActions.fetchServices(123, 0, 20)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create END_SERVICES_LOADING AND SET_ERRORS', () => {
        const store = mockStore(storeFixtures.SERVICES_EMPTY);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(servicesActions.fetchServices(123, 0, 20)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });
});
