/**
 * Настройки webpack для генерации серверного detect'а
 * @file
 */

const path = require('path');
const commonConfig = require('./webpack-common');

const serverConfig = {
    entry: {
        'lib.server': 'src/lib.ts',
        'lib.server.min': 'src/lib.ts',

        'detect.common': 'src/detect.ts',
        'detect.common.min': 'src/detect.ts'
    },
    output: {
        path: path.resolve('./dist'),
        filename: '[name].js',
        libraryTarget: 'commonjs'
    }
};

module.exports = Object.assign({}, commonConfig, serverConfig);
