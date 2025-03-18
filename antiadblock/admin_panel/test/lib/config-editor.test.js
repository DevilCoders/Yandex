import {getConfig, mergeConfig, getSchemaTypeAndDefVal, clearDefaultValues, getStatus} from 'app/lib/config-editor';
import ConfigTextEditor from 'app/components/config-editor/__text/config-editor__text';
import ConfigTupleEditor from 'app/components/config-editor/__tuple/config-editor__tuple';
import ConfigEnumEditor from 'app/components/config-editor/__enum/config-editor__enum';
import ConfigCheckboxGroupEditor from 'app/components/config-editor/__checkbox-group/config-editor__checkbox-group';
import ConfigArrayEditor from 'app/components/config-editor/__array/config-editor__array';
import ConfigTokensEditor from 'app/components/config-editor/__tokens/config-editor__tokens';
import ConfigTagsEditor from 'app/components/config-editor/__tags/config-editor__tags';
import ConfigNumberEditor from 'app/components/config-editor/__number/config-editor__number';
import ConfigObjectEditor from 'app/components/config-editor/__object/config-editor__object';
import ConfigBoolEditor from 'app/components/config-editor/__bool/config-editor__bool';
import ConfigLinkEditor from 'app/components/config-editor/__link/config-editor__link';
import ConfigCryptEditor from 'app/components/config-editor/__crypt/config-editor__crypt';
import ConfigElemsEditor from 'app/components/config-editor/__elems/config-editor__elems';
import ConfigJsonEditor from 'app/components/config-editor/__json/config-editor__json';
import ConfigHtmlEditor from 'app/components/config-editor/__html/config-editor__html';

import {STATUS} from 'app/enums/config';

describe('configEditor', () => {
    describe('getConfig', () => {
        const mockConfig = {
            hint: 'test-hint',
            placeholder: 'test-placeholder',
            title: 'test-title',
            type: '---inputYourType---',
            key: '---inputYourKey--- (Object)',
            value: '---inputYourValue--- (Object)',
            children: {
                placeholder: 'test-children-placeholder',
                type: '---inputYourType---'
            }
        };

        test('get array config', () => {
            mockConfig.type = 'array';
            mockConfig.children.type = 'regexp';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigArrayEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint,
                    childProps: {
                        placeholder: mockConfig.children.placeholder,
                        hint: undefined,
                        title: undefined
                    },
                    Item: ConfigTextEditor
                }
            });
        });

        test('get tokens config', () => {
            mockConfig.type = 'tokens';
            mockConfig.children.type = 'string';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigTokensEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint,
                    childProps: {
                        placeholder: mockConfig.children.placeholder,
                        hint: undefined,
                        title: undefined
                    },
                    Item: ConfigTextEditor
                }
            });
        });

        test('get string config', () => {
            mockConfig.type = 'string';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigTextEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get object config', () => {
            mockConfig.type = 'object';
            mockConfig.key = 'string';
            mockConfig.value = 'string';
            mockConfig.placeholder = {
                key_placeholder: 'test-key-placeholder',
                value_placeholder: 'test-value-placeholder'
            };
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigObjectEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint,
                    childProps: {
                        placeholder: [
                            mockConfig.placeholder.key_placeholder,
                            mockConfig.placeholder.value_placeholder
                        ],
                        items: [ConfigTextEditor, ConfigTextEditor]
                    },
                    Item: ConfigTupleEditor
                }
            });
        });

        test('get tags config', () => {
            mockConfig.type = 'tags';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigTagsEditor,
                props: {
                    placeholder: mockConfig.children.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get select config', () => {
            mockConfig.type = 'select';
            mockConfig.values = {
                testVal1: 0,
                testVal2: 1
            };
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigEnumEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint,
                    enum: mockConfig.values
                }
            });
        });

        test('get multi_checkbox config', () => {
            mockConfig.type = 'multi_checkbox';
            mockConfig.values = [
                {
                    hint: 'test-values-hint',
                    title: 'test-values-title',
                    value: 'test-values-value'
                },
                {
                    hint: 'test-values-hint',
                    title: 'test-values-title',
                    value: 'test-values-value'
                }
            ];
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigCheckboxGroupEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint,
                    enum: mockConfig.values,
                    multi: true
                }
            });
        });

        test('get multi_select config', () => {
            mockConfig.type = 'multi_select';
            mockConfig.values = [
                {
                    hint: 'test-values-hint',
                    title: 'test-values-title',
                    value: 'test-values-value'
                },
                {
                    hint: 'test-values-hint',
                    title: 'test-values-title',
                    value: 'test-values-value'
                }
            ];
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigEnumEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint,
                    enum: mockConfig.values,
                    multi: true
                }
            });
        });

        test('get number config', () => {
            mockConfig.type = 'number';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigNumberEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get bool config', () => {
            mockConfig.type = 'bool';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigBoolEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get link config', () => {
            mockConfig.type = 'link';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigLinkEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get crypt config', () => {
            mockConfig.type = 'crypt_body';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigCryptEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get elems config', () => {
            mockConfig.type = 'detect_elems';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigElemsEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get json config', () => {
            mockConfig.type = 'detect_custom';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigJsonEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get html config', () => {
            mockConfig.type = 'detect_html';
            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigHtmlEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });

        test('get test_data config', () => {
            mockConfig.type = 'test_data';
            mockConfig.placeholder = 'test-data-placeholder';
            mockConfig.hint = 'test-data-hint';
            mockConfig.title = 'test-data-title';

            expect(getConfig(mockConfig)).toEqual({
                constructor: ConfigJsonEditor,
                props: {
                    placeholder: mockConfig.placeholder,
                    title: mockConfig.title,
                    hint: mockConfig.hint
                }
            });
        });
    });

    describe('merge config', () => {
        test('simple merge', () => {
            const parentConfig = {a: 1, b: {c: 1}};
            const childConfig = {b: {a: 2}};
            const mergedConfig = {a: 1, b: {a: 2}};

            expect(mergeConfig(parentConfig, childConfig)).toEqual(mergedConfig);
        });

        test('if parent is empty', () => {
            const parentConfig = {};
            const childConfig = {b: {a: 2}};
            const mergedConfig = {};

            expect(mergeConfig(parentConfig, childConfig)).toEqual(mergedConfig);
        });

        test('if child is empty', () => {
            const parentConfig = {b: {a: 2}};
            const childConfig = {};
            const mergedConfig = {b: {a: 2}};

            expect(mergeConfig(parentConfig, childConfig)).toEqual(mergedConfig);
        });

        test('if unset true', () => {
            const parentConfig = {d: 'aaaa', b: {a: 2}};
            const childConfig = {};
            const mergedConfig = {d: false, b: {a: 2}};
            const schemaDefault = {d: false, b: {}};
            const dataSettings = {d: {UNSET: true}};

            expect(mergeConfig(parentConfig, childConfig, dataSettings, schemaDefault)).toEqual(mergedConfig);
        });

        test('if unset false', () => {
            const parentConfig = {d: 'aaaa', b: {a: 2}};
            const childConfig = {};
            const mergedConfig = {d: 'aaaa', b: {a: 2}};
            const schemaDefault = {d: false, b: {}};
            const dataSettings = {d: {UNSET: false}};

            expect(mergeConfig(parentConfig, childConfig, dataSettings, schemaDefault)).toEqual(mergedConfig);
        });
    });

    describe('getSchemaTypeAndDefVal', () => {
        test('extract schema and get keys and values', () => {
            const schema = [{
                group_name: 'test1',
                items: [{
                    key: {
                        name: 'TEST_KEY1',
                        default: [],
                        type_schema: {
                            type: 'array'
                        }
                    }
                }, {
                    key: {
                        name: 'TEST_KEY2',
                        default: '',
                        type_schema: {
                            type: 'string'
                        }
                    }
                }]
            }, {
                group_name: 'test2',
                items: [{
                    key: {
                        name: 'TEST_KEY3',
                        default: false,
                        type_schema: {
                            type: 'bool'
                        }
                    }
                }]
            }];
            const schemaData = {
                values: {
                    TEST_KEY1: [],
                    TEST_KEY2: '',
                    TEST_KEY3: false
                },
                types: {
                    TEST_KEY1: 'array',
                    TEST_KEY2: 'string',
                    TEST_KEY3: 'bool'
                }
            };

            expect(getSchemaTypeAndDefVal(schema)).toEqual(schemaData);
        });
    });

    describe('clearDefaultValues', () => {
        test('cleared default values from config', () => {
            const config = {
                a: [],
                b: 'asdas',
                d: 1231
            };
            const defValues = {
                a: [],
                b: '',
                c: null
            };
            const settings = {
                b: {
                    UNSET: true
                }
            };
            const result = {
                d: 1231
            };

            expect(clearDefaultValues(config, settings, defValues)).toEqual(result);
        });
    });

    describe('getStatus', () => {
        test('get priority status ACTIVE', () => {
            const statuses = [STATUS.TEST, STATUS.ACTIVE, STATUS.APPROVED];
            const expId = undefined;

            expect(getStatus(statuses, expId)).toEqual(STATUS.ACTIVE);
        });

        test('get priority status TEST', () => {
            const statuses = [STATUS.TEST, STATUS.APPROVED];
            const expId = undefined;

            expect(getStatus(statuses, expId)).toEqual(STATUS.TEST);
        });

        test('get priority status EXPERIMENT', () => {
            const statuses = [STATUS.INACTIVE, STATUS.APPROVED];
            const expId = 123;

            expect(getStatus(statuses, expId)).toEqual(STATUS.EXPERIMENT);
        });

        test('get priority status NOT PRIORITY is empty', () => {
            const statuses = [STATUS.INACTIVE, STATUS.APPROVED];
            const expId = null;

            expect(getStatus(statuses, expId)).toEqual('');
        });
    });
});
