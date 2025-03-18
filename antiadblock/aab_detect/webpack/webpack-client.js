/**
 * Настройки webpack для генерации клентского detect'а
 * @file
 */

const path = require('path');
const commonConfig = require('./webpack-common');

const clientConfig = {
    entry: {
        'lib.browser': 'src/libBrowser.ts',
        'lib.browser.min': 'src/libBrowser.ts'
    },
    output: {
        path: path.resolve('./dist'),
        filename: '[name].js',
        libraryTarget: 'this'
    },
};

module.exports = Object.assign({}, commonConfig, clientConfig);
