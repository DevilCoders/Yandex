
/**
 * Настройки webpack для генерации detect'а
 * @file
 */

const path = require('path');
const commonConfig = require('./webpack-common');

const detectConfig = {
    entry: {
        'detect.global': 'src/detect.ts',
        'detect.global.min': 'src/detect.ts',
    },
    output: {
        path: path.resolve('./dist'),
        filename: '[name].js',
        libraryTarget: 'window'
    }
};

module.exports = Object.assign({}, commonConfig, detectConfig);
