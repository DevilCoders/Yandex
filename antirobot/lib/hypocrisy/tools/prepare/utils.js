const util = require('util');
const v8 = require('v8');

function shuffle(xs) {
    for (let i = 0; i < xs.length - 1; ++i) {
        const j = random(i, xs.length);
        [xs[i], xs[j]] = [xs[j], xs[i]];
    }
}

// Not exactly unbiased but it will do.
function random(min, max) {
    return Math.floor(Math.random() * (max - min)) + min;
}

function randomChoice(xs) {
    return xs[random(0, xs.length)];
}

function extend(arr, xs) {
    for (let x of xs) {
        arr.push(x);
    }
}

function inspect(obj, options) {
    return util.inspect(obj, Object.assign({}, options, {depth: null}));
}

function visit(obj, fun) {
    for (const key in obj) {
        const result = fun(obj[key]);

        if ('substitute' in result) {
            obj[key] = result.substitute;
        }

        if (result.proceed) {
            visit(obj[key], fun);
        }
    }
}

function readLittle32(xs, i) {
    return (xs[i] | (xs[i + 1] << 8) | (xs[i + 2] << 16) | (xs[i + 3] << 24)) >>> 0;
}

function deepCopy(x) {
    return v8.deserialize(v8.serialize(x));
}

function last(xs) {
    return xs[xs.length - 1];
}

function checkNumberOfItems(description, candidates, n) {
    if (candidates.length !== n) {
        throw new Error(`expected ${n} ${description} but found ${candidates.length}`);
    }

    return n == 1 ? candidates[0] : candidates;
}

module.exports = {
    shuffle,
    random,
    randomChoice,
    extend,
    inspect,
    visit,
    readLittle32,
    deepCopy,
    last,
    checkNumberOfItems,
};
