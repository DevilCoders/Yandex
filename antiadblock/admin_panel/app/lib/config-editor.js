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

import {get, isEqual, set} from 'lodash';
import {STATUS} from 'app/enums/config';

const typeMapping = {
    select: ConfigEnumEditor,
    multi_select: ConfigEnumEditor,
    multi_checkbox: ConfigCheckboxGroupEditor,
    regexp: ConfigTextEditor,
    array: ConfigArrayEditor,
    tokens: ConfigTokensEditor,
    tags: ConfigTagsEditor,
    string: ConfigTextEditor,
    number: ConfigNumberEditor,
    object: ConfigObjectEditor,
    bool: ConfigBoolEditor,
    link: ConfigLinkEditor,
    crypt_body: ConfigCryptEditor,
    detect_elems: ConfigElemsEditor,
    detect_custom: ConfigJsonEditor,
    detect_html: ConfigHtmlEditor,
    test_data: ConfigJsonEditor,
    json: ConfigJsonEditor
};

export function getConfig(config) {
    const answer = {
        constructor: typeMapping[config.type] || ConfigTextEditor,
        props: {
            placeholder: config.placeholder,
            hint: config.hint,
            title: config.title
        }
    };

    let child;

    // custom config
    switch (config.type) {
        // TODO костыль пока не будет выпилен совсем ANTIADB-2130
        case 'test_data':
            answer.props.title = 'test-data-title';
            answer.props.hint = 'test-data-hint';
            answer.props.placeholder = 'test-data-placeholder';
            break;
        case 'select':
            answer.props.enum = config.values;
            break;
        case 'multi_checkbox':
        case 'multi_select':
            answer.props.enum = config.values;
            answer.props.multi = true;
            break;
        case 'object':
            answer.props.Item = ConfigTupleEditor;
            answer.props.childProps = {
                items: [typeMapping[config.key], typeMapping[config.value]],
                placeholder: [
                    config.placeholder.key_placeholder,
                    config.placeholder.value_placeholder
                ]
            };
            break;
        case 'tokens':
        case 'array':
            child = getConfig(config.children);
            answer.props.childProps = child.props;
            answer.props.Item = child.constructor;
            break;
        case 'tags':
            answer.props.placeholder = config.children.placeholder;
            break;
        default:
    }

    return answer;
}

export function getDefaultValue(schema, name) {
    for (let i = 0; i < schema.length; i++) {
        const items = schema[i].items;
        for (let j = 0; j < items.length; j++) {
            const item = items[j].key;

            if (item.name === name) {
                return item.default;
            }
        }
    }
    return undefined;
}

export function isEmptyValue(type, value, schema, name) {
    switch (type) {
        case 'select':
            return getDefaultValue(schema, name) === value;
        case 'array':
        case 'tags':
        case 'multi_checkbox':
        case 'tokens':
            return !value.length;
        case 'object':
        case 'json':
        case 'test_data':
            return !value && !Object.keys(value).length;
        case 'bool':
            return !value;
        case 'detect_html':
        case 'string':
            return !value;
        case 'number':
            return isNaN(value);
        default:
            return true;
    }
}

export function getSchemaTypeAndDefVal(schema) {
    return schema.reduce((obj, group) => {
        group.items.forEach(item => {
            obj.values[item.key.name] = item.key.default;
            obj.types[item.key.name] = item.key.type_schema.type;
        });

        return obj;
    }, {
        values: {},
        types: {}
    });
}

export function mergeConfig(parent, current, dataSettings, schema) {
    const mergedConfig = {};

    for (let [key, valueP] of Object.entries(parent)) {
        if (get(dataSettings, [key, 'UNSET'], false)) {
            mergedConfig[key] = schema[key];
        } else {
            mergedConfig[key] = (current[key] !== undefined && current[key] !== null) ? current[key] : valueP;
        }
    }

    return mergedConfig;
}

// TODO убрать эту функцию, когда иеархические конфиги приведут в порядок
// На текущий момент с бека могут прилетать в текущем конфиге данные,
// которые равняются дефолтным значениям схемы, это последствия старых конфигов.
// После создания иеархии проблема должна уйти
// Предположительно, можно убрать функцию через месяц (8 марта 2020)
export function clearDefaultValues(dataCurrent = {}, dataSettings = {}, schema = {}) {
    return Object.keys(dataCurrent).reduce((newData, key) => {
        if (!isEqual(dataCurrent[key], schema[key]) && !get(dataSettings, [key, 'UNSET'], false)) {
            set(newData, key, get(dataCurrent, key));
        }

        return newData;
    }, {});
}

export function getStatus(statuses, isExp) {
    return isExp ? STATUS.EXPERIMENT : statuses.reduce((status, item) => (
        item === STATUS.ACTIVE ? item :
            item === STATUS.TEST && status !== STATUS.ACTIVE ? item : status
    ), '');
}
