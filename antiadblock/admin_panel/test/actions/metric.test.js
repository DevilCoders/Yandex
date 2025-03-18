import * as metricActions from 'app/actions/metric';
import * as metricActionsFixtures from '../fixtures/actions/metric';

import configureMockStore from 'redux-mock-store';
import thunk from 'redux-thunk';
import {METRIC_HTTP_CODES} from '../fixtures/backend/metric';

const middlewares = [thunk];
const mockStore = configureMockStore(middlewares);

describe('metric actions', () => {
    const serviceId = 'test.service';
    const metricName = 'http_codes';
    const range = 336;
    const loaded = true;
    const emptyStore = {};

    it('should create START_METRIC_LOADING', () => {
        expect(metricActions.startMetricLoading(serviceId, metricName, range)).toEqual(metricActionsFixtures.START_METRIC_LOADING);
    });

    it('should create END_METRIC_LOADING', () => {
        expect(metricActions.endMetricLoading(serviceId, metricName, range, METRIC_HTTP_CODES, loaded)).toEqual(metricActionsFixtures.END_METRIC_LOADING);
    });

    it('should create START_METRIC_LOADING after loading', () => {
        const store = mockStore(emptyStore);

        fetch.mockResponseOnce(JSON.stringify(METRIC_HTTP_CODES));

        return store.dispatch(metricActions.fetchMetric(serviceId, metricName, range)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create START_METRIC_LOADING after loading and return false to loaded flag', () => {
        const store = mockStore(emptyStore);

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(metricActions.fetchMetric(serviceId, metricName, range)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });
});
