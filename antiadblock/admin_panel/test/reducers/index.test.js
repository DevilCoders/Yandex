import {getService, getServices, getConfigPreview, getConfigDiffModal, getConfigApplying, getMetric, getErrors} from 'app/reducers/index';

import {ROOT_STATE} from '../fixtures/store';

describe('index', () => {
    const initialState = ROOT_STATE;

    // TODO test combine reducers

    describe('selectors', () => {
        test('getService should return service obj', () => {
            expect(getService(initialState)).toMatchSnapshot();
        });

        test('getServices should return services obj', () => {
            expect(getServices(initialState)).toMatchSnapshot();
        });

        test('getConfigPreview should return configPreview obj', () => {
            expect(getConfigPreview(initialState)).toMatchSnapshot();
        });

        test('getConfigApplying should return configApplying obj', () => {
            expect(getConfigApplying(initialState)).toMatchSnapshot();
        });

        test('getConfigDiffModal should return configDiffModal obj', () => {
            expect(getConfigDiffModal(initialState)).toMatchSnapshot();
        });

        test('getMetric should return metric obj', () => {
            expect(getMetric(initialState, 'test.service')).toMatchSnapshot();
        });

        test('getErrors should return errors obj', () => {
            expect(getErrors(initialState)).toMatchSnapshot();
        });
    });
});
