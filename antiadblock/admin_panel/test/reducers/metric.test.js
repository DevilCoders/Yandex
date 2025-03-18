import reducer from 'app/reducers/metric';
import * as metricActionFixtures from '../fixtures/actions/metric';
import * as serviceActionFixtures from '../fixtures/actions/service';
import {METRIC} from '../fixtures/store';

describe('metric', () => {
    const initialState = {};

    describe('reducer', () => {
        it('should return the initial state', () => {
            expect(reducer(undefined, {})).toMatchSnapshot();
        });

        it('should handle START_METRIC_LOADING', () => {
            expect(
                reducer(initialState, metricActionFixtures.START_METRIC_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_METRIC_LOADING', () => {
            const afterMetricLoadingStarted = reducer(initialState, metricActionFixtures.START_METRIC_LOADING);
            const afterMetricLoaded = reducer(afterMetricLoadingStarted, metricActionFixtures.END_METRIC_LOADING);

            expect(afterMetricLoaded).toMatchSnapshot();
        });

        it('should handle END_SERVICE_LOADING', () => {
            const afterServiceConfigsLoaded = reducer(METRIC, serviceActionFixtures.END_SERVICE_LOADING);

            expect(afterServiceConfigsLoaded).toMatchSnapshot();
        });

        it('should handle START_METRIC_LOADING and remove data from store', () => {
            const afterServiceConfigsLoaded = reducer(METRIC, metricActionFixtures.START_METRIC_LOADING);

            expect(afterServiceConfigsLoaded).toMatchSnapshot();
        });

        it('should handle END_SERVICE_LOADING and store should not changed', () => {
            const afterMetricLoaded = reducer(METRIC, metricActionFixtures.END_METRIC_LOADING);

            expect(afterMetricLoaded).toMatchSnapshot();
        });
    });
});
