import * as schemaActions from 'app/actions/schema';
import * as schemaActionsFixtures from '../fixtures/actions/schema';

import configureMockStore from 'redux-mock-store';
import thunk from 'redux-thunk';
import {SCHEMA} from '../fixtures/backend/schema';

const middlewares = [thunk];
const mockStore = configureMockStore(middlewares);

describe('schema actions', () => {
    const labelId = 'test.service';

    it('should create START_SCHEMA_LOADING', () => {
        expect(schemaActions.startSchemaLoading(labelId)).toEqual(schemaActionsFixtures.START_SCHEMA_LOADING);
    });

    it('should create END_SCHEMA_LOADING', () => {
        expect(schemaActions.endSchemaLoading(labelId, SCHEMA)).toEqual(schemaActionsFixtures.END_SCHEMA_LOADING);
    });

    it('should create START_SCHEMA_LOADING after loading', () => {
        const store = mockStore({});

        fetch.mockResponseOnce(JSON.stringify(SCHEMA));

        return store.dispatch(schemaActions.fetchSchema(labelId)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });

    it('should create SET_ERRORS after catched an error', () => {
        const store = mockStore({});

        fetch.mockRejectOnce(new Error('Test error'));

        return store.dispatch(schemaActions.fetchSchema(labelId)).then(() => {
            expect(store.getActions()).toMatchSnapshot();
        });
    });
});
