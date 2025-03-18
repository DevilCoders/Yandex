export function getByPath(obj, path) {
    return path.reduce((acc, item) => {
        // item может быть Number, в том числе и 0. Пропускаем только null | undefined | ''
        if (acc && item !== undefined && item !== null && item !== '') {
            return acc[item];
        }

        return acc;
    }, obj);
}

export function setByPath(obj, path, value) {
    if (!obj || !path || !path.length) {
        return;
    }

    const hasOwn = Object.prototype.hasOwnProperty;

    const child = path[0];

    if (path.length === 1) {
        obj[child] = value;
        return;
    }

    if (!hasOwn.call(obj, child)) {
        obj[child] = {};
    }

    setByPath(obj[child], path.slice(1), value);
}
