const PLURAL_INDICES = {
    none: 3,
    one: 0,
    some: 1,
    many: 2
};

function i18n(keyset, key, count) {
    let translates, result;

    if (process.env.BEM_LANG === 'ru') {
        translates = require(`../i18n/${keyset}/${keyset}.i18n/ru`);
    } else if (process.env.BEM_LANG === 'en') {
        translates = require(`../i18n/${keyset}/${keyset}.i18n/en`);
    }

    if (!translates[keyset] || !translates[keyset][key]) {
        return key;
    }

    result = translates[keyset][key];

    if (Array.isArray(result)) {
        count = count || 0;
        const lastNumber = count % 10;
        const lastNumbers = count % 100;

        if (lastNumber === 0) {
            return result[PLURAL_INDICES.none];
        }

        if (lastNumber === 1 && lastNumbers !== 11) {
            return result[PLURAL_INDICES.one];
        }

        if ((lastNumber > 1 && lastNumber < 5) && (lastNumbers < 10 || lastNumbers > 20)) {
            return result[PLURAL_INDICES.some];
        }

        return result[PLURAL_INDICES.many];
    }

    return result;
}

export default i18n;
