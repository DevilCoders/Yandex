
/**
 * Настройки webpack для генерации detect'а
 * @file
 */

const path = require('path');
const webpack = require('webpack');
const commonConfig = require('./webpack-common');

const detectConfig = {
    entry: {
        'inject': 'src/inject.ts',
        'inject.min': 'src/inject.ts',
    },
    output: {
        path: path.resolve('./dist'),
        filename: '[name].js',
        libraryTarget: 'window'
    },
    plugins: [
        ...commonConfig.plugins,
        new webpack.DefinePlugin({
            __disableLogging__: true,
            __disableIframeDetection__: true,
            __disableTypeDetection__: true,
            __disableCookiematching__: true,
        })
    ]
};

module.exports = Object.assign({}, commonConfig, detectConfig);
