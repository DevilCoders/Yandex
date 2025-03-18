import reducer from 'app/reducers/service/moderating-config';
import * as configModeratingActionsFixtures from '../fixtures/actions/config-moderating';
import {START_SERVICE_CONFIG_APPROVING, END_SERVICE_CONFIG_APPROVING, START_SERVICE_CONFIG_DECLINING, END_SERVICE_CONFIG_DECLINING} from '../fixtures/store';

describe('config-moderating reducer', () => {
    const startState = START_SERVICE_CONFIG_APPROVING;
    const endState = END_SERVICE_CONFIG_APPROVING;

    it('should return the initial state', () => {
        expect(reducer(undefined, {})).toMatchSnapshot();
    });

    it('should handle START_SERVICE_CONFIG_MODERATING', () => {
        expect(
            reducer(startState, configModeratingActionsFixtures.START_SERVICE_CONFIG_APPROVING)
        ).toMatchSnapshot();
    });

    it('should handle END_SERVICE_CONFIG_MODERATING', () => {
        expect(
            reducer(endState, configModeratingActionsFixtures.END_SERVICE_CONFIG_APPROVING)
        ).toMatchSnapshot();
    });

    const startState2 = START_SERVICE_CONFIG_DECLINING;
    const endState2 = END_SERVICE_CONFIG_DECLINING;

    it('should return the initial state', () => {
        expect(reducer(undefined, {})).toMatchSnapshot();
    });

    it('should handle START_SERVICE_CONFIG_MODERATING', () => {
        expect(
            reducer(startState2, configModeratingActionsFixtures.START_SERVICE_CONFIG_DECLINING)
        ).toMatchSnapshot();
    });

    it('should handle END_SERVICE_CONFIG_MODERATING', () => {
        expect(
            reducer(endState2, configModeratingActionsFixtures.END_SERVICE_CONFIG_DECLINING)
        ).toMatchSnapshot();
    });
});
