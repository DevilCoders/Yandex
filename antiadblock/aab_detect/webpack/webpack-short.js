/**
 * Версия конфига без использования детекта на ботов
 * @file
 */

const webpack = require('webpack');
const path = require('path');
const commonConfig = require('./webpack-common');
const uglifyJs = require('./uglify-settings');

const detectShortConfig = {
    entry: {
        'detect.global.no.bots': 'src/detect.ts',
        'detect.global.no.bots.min': 'src/detect.ts'
    },
    output: {
        path: path.resolve('./dist'),
        filename: '[name].js',
        libraryTarget: 'window'
    },
    plugins: [
        new webpack.DefinePlugin({
            __bundle__: '"adblock"',
            __dev__: true,
            __protocol__: 'https:'
        }),
        uglifyJs
    ]
};


module.exports = Object.assign({}, commonConfig, detectShortConfig);
