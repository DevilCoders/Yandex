const ELEM_PREFIX = '__';
const MOD_PREFIX = '_';
const MOD_VALUE_PREFIX = '_';

function createMods(block, mods) {
    let classNames = [];

    if (mods) {
        Object.keys(mods).forEach(mod => {
            if (mods[mod]) {
                const modString = mods[mod] === true ?
                    mod :
                    `${mod}${MOD_VALUE_PREFIX}${mods[mod]}`;

                classNames.push(`${block}${MOD_PREFIX}${modString}`);
            }
        });
    }

    return classNames;
}

function bemClasses(block, elem, mods) {
    const blockElem = `${block}${elem ? `${ELEM_PREFIX}${elem}` : ''}`;

    return [blockElem, ...createMods(blockElem, mods)];
}

export default function bemClassesCreator(block, elem, mods, mix) {
    let classNames = bemClasses(block, elem, mods);

    if (mix) {
        if (Array.isArray(mix)) {
            mix.forEach(mixItem => {
                if (mixItem) {
                    classNames = classNames.concat(bemClasses(mixItem.block, mixItem.elem, mixItem.mods));
                }
            });
        } else {
            classNames = classNames.concat(bemClasses(mix.block, mix.elem, mix.mods));
        }
    }

    return classNames.join(' ');
}
