import {newContext} from 'immutability-helper';

// TODO: сократить список
import setWith from 'lodash/setWith';
import cloneDeep from 'lodash/cloneDeep';
import unset from 'lodash/unset';
import isEqual from 'lodash/isEqual';
import reject from 'lodash/reject';
import findIndex from 'lodash/findIndex';

const update = newContext();

update.extend('$byPath', (data, original) => {
    return update(original, setWith({}, data.path, data.do, Object));
});

update.extend('$unsetByPath', (paths, original) => {
    const cloned = cloneDeep(original);

    if (!Array.isArray(paths)) {
        paths = [paths];
    }

    paths.forEach(path => {
        unset(cloned, path);
    });

    return cloned;
});

update.extend('$unsetByValue', (values, original) => {
    if (!Array.isArray(values)) {
        values = [values];
    }

    return reject(original, item => {
        return values.find(value => isEqual(value, item)) !== undefined;
    });
});

update.extend('$initArray', (value, original) => {
    return update(original || [], value);
});

update.extend('$initAndPush', (value, original) => {
    return update(original, {
        $initArray: {
            $push: Array.isArray(value) ? value : [value]
        }
    });
});

update.extend('$toggle', (unused, original) => {
    return update(original, {
        $apply: value => !value
    });
});

update.extend('$byId', (data, original) => {
    if (data.path) {
        return update(original, {
            $byPath: {
                path: data.path,
                do: {
                    $byId: {
                        key: data.key,
                        id: data.id,
                        fallback: data.fallback,
                        do: data.do
                    }
                }
            }
        });
    }

    const key = data.key || 'id';
    const index = findIndex(original, {
        [key]: data.id
    });

    if (index === -1) {
        return update(original, data.fallback);
    }

    return update(original, {
        [index]: data.do
    });
});

export default update;
