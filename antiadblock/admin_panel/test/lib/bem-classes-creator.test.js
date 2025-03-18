import bemClassesCreator from 'app/lib/bem-classes-creator';

describe('bem-classes-creator', () => {
    describe('bemClassesCreator', () => {
        const testBlock = 'testBlock';
        const testElem = 'testElem';
        const testMod = 'testMod';
        const someMods = {
            testMod1: true,
            testMod2: true
        };

        test('test return block class', () => {
            expect(bemClassesCreator(testBlock)).toEqual(testBlock);
        });

        test('test return block__elem class', () => {
            expect(bemClassesCreator(testBlock, testElem)).toEqual(`${testBlock}__${testElem}`);
        });

        test('test return block__elem_mod', () => {
            expect(bemClassesCreator(testBlock, testElem, {[testMod]: true})).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem}_${testMod}`);
        });

        test('test return block__elem with false mod', () => {
            expect(bemClassesCreator(testBlock, testElem, {[testMod]: false})).toEqual(`${testBlock}__${testElem}`);
        });

        test('test return block__elem with some mods', () => {
            expect(bemClassesCreator(testBlock, testElem, someMods)).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem}_testMod1 ${testBlock}__${testElem}_testMod2`);
        });

        test('test return block class with mix block', () => {
            expect(bemClassesCreator(testBlock, null, null, {block: testBlock})).toEqual(`${testBlock} ${testBlock}`);
        });

        test('test return block__elem class with mix block', () => {
            expect(bemClassesCreator(testBlock, testElem, null, {block: testBlock})).toEqual(`${testBlock}__${testElem} ${testBlock}`);
        });

        test('test return block__elem class with array mix', () => {
            expect(bemClassesCreator(testBlock, testElem, null, [{block: testBlock}])).toEqual(`${testBlock}__${testElem} ${testBlock}`);
        });

        test('test return block__elem class with mix block', () => {
            expect(bemClassesCreator(testBlock,
                testElem,
                null,
                {
                    block: testBlock,
                    elem: testElem
                })).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem}`);
        });

        test('test return block__elem_mod class with mix block', () => {
            expect(bemClassesCreator(testBlock,
                testElem,
                {
                    [testMod]: true
                },
                {
                    block: testBlock,
                    elem: testElem
                })).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem}_${testMod} ${testBlock}__${testElem}`);
        });

        test('test return block__elem with false mod class with mix block', () => {
            expect(bemClassesCreator(testBlock,
                testElem,
                {
                    [testMod]: false
                },
                {
                    block: testBlock,
                    elem: testElem
                })).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem}`);
        });

        test('test return block__elem with some mods class with mix block', () => {
            expect(bemClassesCreator(testBlock,
                testElem,
                someMods,
                {
                    block: testBlock,
                    elem: testElem
                })).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem}_testMod1 ${testBlock}__${testElem}_testMod2 ${testBlock}__${testElem}`);
        });

        test('test return block__elem with some mods class with mix block', () => {
            expect(bemClassesCreator(testBlock,
                testElem,
                null,
                {
                    block: testBlock,
                    elem: testElem,
                    mods: {
                        testMod: true
                    }
                })).toEqual(`${testBlock}__${testElem} ${testBlock}__${testElem} ${testBlock}__${testElem}_${testMod}`);
        });
    });
});
