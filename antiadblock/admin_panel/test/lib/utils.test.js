
import cloneDeep from 'lodash/cloneDeep';
import {getByPath, setByPath} from 'app/lib/utils';

describe('utils', () => {
    describe('getByPath', () => {
        it('should return undefined, if get undefined as first parameter', () => {
            const result = getByPath(undefined, ['a', 'b', 'c', 'd']);

            expect(result).toEqual(undefined);
        });

        it('should return undefined, if path does not exist', () => {
            const result = getByPath({a: {b: {}}}, ['a', 'b', 'c', 'd']);

            expect(result).toEqual(undefined);
        });

        it('should return result by path', () => {
            const result = getByPath({a: {b: {c: {d: 'result'}}}}, ['a', 'b', 'c', 'd']);

            expect(result).toEqual('result');
        });

        it('should works with arrays', () => {
            const result = getByPath({a: {b: [1, 2, {c: [{d: 'result'}]}]}}, ['a', 'b', 2, 'c', 0, 'd']);

            expect(result).toEqual('result');
        });
    });

    describe('setByPath', () => {
        const initialObj = {
            a: {
                b: {
                    c: {
                        d: [
                            {
                                a: {
                                    b: 'initital'
                                },
                                c: 111
                            }
                        ]
                    },
                    e: 'initial'
                }
            }
        };

        it('should left obj without changes if path is null', () => {
            const testObj = cloneDeep(initialObj);
            setByPath(testObj, null, 'value');

            expect(testObj).toEqual(initialObj);
        });

        it('should left obj without changes if path is empty', () => {
            const testObj = cloneDeep(initialObj);
            setByPath(testObj, [], 'value');

            expect(testObj).toEqual(initialObj);
        });

        it('should not fail with null obj', () => {
            const testObj = null;
            setByPath(testObj, null, 'value');

            expect(testObj).toBe(null);
        });

        it('should return obj with changes by path', () => {
            const testObj = cloneDeep(initialObj);
            const path = ['a', 'b', 'e'];
            const value = 'value';
            const initialValue = 'initial';

            setByPath(testObj, path, value);
            expect(getByPath(testObj, path)).toEqual(value);

            // rollback
            setByPath(testObj, path, initialValue);
            expect(testObj).toEqual(initialObj);
        });

        it('should create props if they does not exist', () => {
            const testObj = {};
            const expected = {a: {b: {c: {d: 'value'}}}};
            const path = ['a', 'b', 'c', 'd'];
            const value = 'value';

            setByPath(testObj, path, value);
            expect(testObj).toEqual(expected);
        });

        it('should works with arrays', () => {
            const testObj = cloneDeep(initialObj);
            const path = ['a', 'b', 'c', 'd', 0, 'c'];
            const value = '555';
            const initialValue = 111;

            setByPath(testObj, path, value);
            expect(getByPath(testObj, path)).toEqual(value);

            // rollback
            setByPath(testObj, path, initialValue);
            expect(testObj).toEqual(initialObj);
        });
    });
});
