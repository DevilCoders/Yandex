const clientConfig = require('./webpack/webpack-client');
const injectBuild = require('./webpack/webpack-inject');
const minimalBuild = require('./webpack/builds/minimal');
const internalBuild = require('./webpack/builds/internal');
const externalBuild = require('./webpack/builds/external');

let configs = [];

switch (process.env.NODE_ENV) {
    case 'detect':
    default:
        configs = [
            clientConfig,
            minimalBuild,
            internalBuild,
            externalBuild,
            injectBuild
        ];
}

module.exports = configs;
