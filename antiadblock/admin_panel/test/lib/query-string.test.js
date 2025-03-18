import * as qs from 'app/lib/query-string';

describe('query-string', () => {
    describe('camelCaseToSnake', () => {
        test('should get camelCase from snakeCase', () => {
            expect(qs.camelCaseToSnake('helloItsTest')).toEqual('hello_its_test');
        });
    });

    describe('snakeCaseToCamel', () => {
        test('should get snakeCase from camelCase', () => {
            expect(qs.snakeCaseToCamel('hello_its_test')).toEqual('helloItsTest');
        });
    });

    describe('getQueryArgsFromObject', () => {
        test('should get queryArgs from object', () => {
            const mockObj = {
                testParamFirst: 1,
                testParamSecond: 2
            };
            expect(qs.getQueryArgsFromObject(mockObj)).toEqual('?testParamFirst=1&testParamSecond=2');
        });

        test('should get queryArgs from object with convert camelCase to snakeCase', () => {
            const mockObj = {
                testParamFirst: 1,
                testParamSecond: 2
            };
            expect(qs.getQueryArgsFromObject(mockObj, true)).toEqual('?test_param_first=1&test_param_second=2');
        });

        test('should get empty string from empty object', () => {
            const mockObj = {};
            expect(qs.getQueryArgsFromObject(mockObj)).toEqual('');
        });

        test('should get empty string without param in function', () => {
            expect(qs.getQueryArgsFromObject()).toEqual('');
        });
    });

    describe('fromEntries', () => {
        test('should get object from queryArgs', () => {
            const mockQueryString = '?testParamFirst=1&testParamSecond=2';
            const urlParams = new URLSearchParams(mockQueryString);
            const entries = urlParams.entries();

            expect(qs.fromEntries(entries)).toEqual({
                testParamFirst: '1',
                testParamSecond: '2'
            });
        });

        test('should get object from queryArgs with convert snakeCase to camelCase', () => {
            const mockQueryString = '?test_param_first=1&test_param_second=2';
            const urlParams = new URLSearchParams(mockQueryString);
            const entries = urlParams.entries();

            expect(qs.fromEntries(entries, true)).toEqual({
                testParamFirst: '1',
                testParamSecond: '2'
            });
        });

        test('should get empty object from empty queryArgs', () => {
            const mockQueryString = '';
            const urlParams = new URLSearchParams(mockQueryString);
            const entries = urlParams.entries();

            expect(qs.fromEntries(entries)).toEqual({});
        });
    });

    describe('convertDataFromStringToType', () => {
        test('should convert string true to boolean true', () => {
            const mockData = 'true';

            expect(qs.convertDataFromStringToType(mockData, 'bool')).toEqual(true);
        });

        test('should convert string false to boolean false', () => {
            const mockData = 'false';

            expect(qs.convertDataFromStringToType(mockData, 'bool')).toEqual(false);
        });

        test('should convert string to number', () => {
            const mockData = '2313';

            expect(qs.convertDataFromStringToType(mockData, 'number')).toEqual(2313);
        });
    });

    describe('prepareParamsToType', () => {
        test('should return only the parameters that are in the types', () => {
            const mockParams = {
                a: '132',
                b: 'asas',
                c: 'sdasda',
                d: 'true',
                e: 'false'
            };
            const types = {
                a: 'number',
                c: 'string',
                d: 'bool',
                e: 'bool'
            };

            expect(qs.prepareParamsToType(mockParams, types)).toEqual({
                a: 132,
                c: 'sdasda',
                d: true,
                e: false
            });
        });

        test('should return empty object if in function first argument undefined', () => {
            const types = {
                a: 'number',
                c: 'string',
                d: 'bool',
                e: 'bool'
            };

            expect(qs.prepareParamsToType(undefined, types)).toEqual({});
        });

        test('should return empty object if in function second argument undefined', () => {
            const mockParams = {
                a: '132',
                b: 'asas',
                c: 'sdasda',
                d: 'true',
                e: 'false'
            };

            expect(qs.prepareParamsToType(mockParams)).toEqual({});
        });
    });
});
