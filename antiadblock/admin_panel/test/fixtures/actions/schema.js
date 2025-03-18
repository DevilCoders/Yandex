import {SCHEMA} from '../backend/schema';

export const START_SCHEMA_LOADING = {
    type: 'START_SCHEMA_LOADING',
    labelId: 'test.service'
};

export const END_SCHEMA_LOADING = {
    type: 'END_SCHEMA_LOADING',
    labelId: 'test.service',
    schema: SCHEMA
};
