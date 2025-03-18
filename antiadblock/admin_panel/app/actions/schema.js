import serviceApi from 'app/api/service';
import {setGlobalErrors} from 'app/actions/errors';

export const START_SCHEMA_LOADING = 'START_SCHEMA_LOADING';
export const END_SCHEMA_LOADING = 'END_SCHEMA_LOADING';

export function fetchSchema(labelId) {
    return dispatch => {
        dispatch(startSchemaLoading(labelId));

        return serviceApi.fetchSchema(labelId).then(schema => {
            return dispatch(endSchemaLoading(labelId, schema));
        }, error => {
            return dispatch(setGlobalErrors([error.message]));
        });
    };
}

export function startSchemaLoading(labelId) {
    return {
        type: START_SCHEMA_LOADING,
        labelId
    };
}

export function endSchemaLoading(labelId, schema) {
    return {
        type: END_SCHEMA_LOADING,
        labelId,
        schema
    };
}
