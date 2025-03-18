/**
 * Настройки webpack для генерации клентского detect'а
 * @file
 */

const path = require('path');
const webpack = require('webpack');
const clientConfig = require('../webpack-client');

const config = Object.assign({}, clientConfig, {
    output: {
        path: path.resolve('./dist'),
        filename: 'minimal.[name].js',
        libraryTarget: 'this'
    },
    plugins: [
        ...clientConfig.plugins,
        new webpack.DefinePlugin({
            __disableLogging__: true,
            __disableIframeDetection__: true,
            __disableNetworkDetection__: true,
            __disableTypeDetection__: true,
            __disableCookiematching__: true,
        })
    ]
});

module.exports = config;