import {isNull, isNaN} from 'lodash';

export function camelCaseToSnake(string) {
    return string.replace(/[\w]([A-Z])/g, m => (m[0] + '_' + m[1])).toLowerCase();
}

export function getQueryArgsFromObject(obj = {}, useCamelCaseToSnake) {
    const queryArr = Object.keys(obj).reduce((arr, k) => {
        if (!isNaN(obj[k]) && !isNull(obj[k])) {
            const convertedKey = useCamelCaseToSnake ? camelCaseToSnake(k) : k;

            arr.push(`${encodeURIComponent(convertedKey)}=${encodeURIComponent(obj[k])}`);
        }

        return arr;
    }, []);

    if (!queryArr.length) {
        return '';
    }

    return '?' + queryArr.join('&');
}

export function snakeCaseToCamel(value) {
    return value.replace(/_\w/g, m => m[1].toUpperCase());
}

export function fromEntries(entries, useConvertSnakeToCamel = false) {
    let result = {};

    for (let entry of entries) {
        const [key, value] = entry;
        const convertedKey = useConvertSnakeToCamel ? snakeCaseToCamel(key) : key;

        result[convertedKey] = value;
    }
    return result;
}

export function convertDataFromStringToType(data, type) {
    let convertedData = data;

    switch (type) {
        case 'number':
            convertedData = Number.parseInt(data, 10);
            break;
        case 'bool':
            convertedData = data === 'true';
            break;
        default:
            break;
    }

    return convertedData;
}

export function prepareParamsToType(params = {}, paramsTypes = {}) {
    if (!Object.keys(params).length || !Object.keys(paramsTypes).length) {
        return {};
    }

    return Object.keys(paramsTypes).reduce((res, key) => {
        if (params[key]) {
            res[key] = convertDataFromStringToType(params[key], paramsTypes[key]);
        }

        return res;
    }, {});
}
