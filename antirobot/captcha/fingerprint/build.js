const fs = require('fs');
const path = require('path');

const PATH = './node_modules/@yandex-market/greed/dist';
const buildType = 'umd'; // Тип сборки. Возможны значения 'commonjs' и 'debug'
const checksType = 'all'; // Какой набор фингерпринтов нужен. Возможны значения 'light' и 'heavy'

const greedScript = fs.readFileSync(path.join(__dirname, PATH, `${checksType}.${buildType}.js`), 'utf-8');
const greedMappingJson = JSON.parse(fs.readFileSync(path.join(__dirname, PATH, 'mappings.json'), 'utf-8'));
let assembledScript = `
(function (self) {
    ${greedScript};
})(window);
`;

fs.writeFileSync(path.join(__dirname, './server/generated/js/greed.js'), assembledScript);

const inverseMapping = {};
for (let shortName of Object.keys(greedMappingJson)) {
    const longName = greedMappingJson[shortName];
    inverseMapping[longName.replace(/\./ig, '_')] = shortName;
}
fs.writeFileSync(path.join(__dirname, './server/generated/mapping.json'), JSON.stringify(inverseMapping, null, 4));
