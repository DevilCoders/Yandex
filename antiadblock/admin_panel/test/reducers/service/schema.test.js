import reducer from 'app/reducers/service/schema';
import * as serviceActionFixtures from '../../fixtures/actions/service';
import * as schemaActionFixtures from '../../fixtures/actions/schema';
import {SCHEMA} from '../../fixtures/store';

describe('schema', () => {
    const initialState = SCHEMA;

    describe('reducer', () => {
        it('should return the initial state', () => {
            expect(reducer(undefined, {})).toMatchSnapshot();
        });

        it('should handle START_SERVICE_LOADING', () => {
            expect(
                reducer(initialState, serviceActionFixtures.START_SERVICE_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle START_SCHEMA_LOADING', () => {
            expect(
                reducer(initialState, schemaActionFixtures.START_SCHEMA_LOADING)
            ).toMatchSnapshot();
        });

        it('should handle END_SCHEMA_LOADING', () => {
            const afterSchemaLoadingStarted = reducer(initialState, schemaActionFixtures.START_SCHEMA_LOADING);
            const afterSchemaLoaded = reducer(afterSchemaLoadingStarted, schemaActionFixtures.END_SCHEMA_LOADING);

            expect(afterSchemaLoaded).toMatchSnapshot();
        });
    });
});
