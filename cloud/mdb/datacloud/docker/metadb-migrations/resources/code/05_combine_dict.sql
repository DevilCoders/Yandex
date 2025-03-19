CREATE EXTENSION IF NOT EXISTS plv8;
CREATE OR REPLACE FUNCTION code.combine_dict(
    i_data jsonb[]
) RETURNS jsonb AS
$$
    function isMapping(obj) {
        return typeof obj === 'object' && obj !== null && !Array.isArray(obj);
    }

    function _merge_by_keys(container, source) {
        for (let prop in source) {
            if (!Object.prototype.hasOwnProperty.call(source, prop)) {
                continue
            }
            let newValue = source[prop];
            let valueInContainer = container[prop];
            if (isMapping(newValue) && isMapping(valueInContainer)) {
                container[prop] = _merge_by_keys(valueInContainer, newValue)
            } else {
                container[prop] = newValue
            }
        }
        return container
    }

    function _combine_dicts(source_array) {
        const result = {}
        for (let i = 0; i < source_array.length; i++) {
            _merge_by_keys(result, source_array[i])
        }
        return result
    }

    return JSON.stringify(_combine_dicts(i_data))
$$
LANGUAGE 'plv8' IMMUTABLE STRICT;
