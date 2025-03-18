/**
 * Общие настройки webpack
 * @file
 */

const webpack = require('webpack');
const path = require('path');
const uglifyJs = require('./uglify-settings');

module.exports = {
    resolve: {
        modules: [
            path.resolve('./'),
            'node_modules'
        ],
        extensions: ['.ts', '.js'],
        alias: {
            'common-pcode': path.resolve('./pcode/src/common-pcode'),
            logger: path.resolve('./pcode/src/logger'),
            libs: path.resolve('./pcode/src/libs'),
            global: path.resolve('./pcode/src/global.ts'),
        },
    },
    module: {
        loaders: [{
            test: /\.ts$/,
            rules: [
                {
                    test: /\.ts(x)?$/,
                    use: [
                        `babel-loader?${JSON.stringify({
                            plugins: [
                                ['transform-runtime', {
                                    helpers: false,
                                    polyfill: false,
                                    regenerator: true,
                                    moduleName: 'babel-runtime'
                                }],
                                'es6-promise',
                                'transform-object-assign',
                                'transform-undefined-to-void'
                            ],
                            presets: ['es2015']
                        })}`,
                        `awesome-typescript-loader?${JSON.stringify({
                            tsconfig: path.resolve(__dirname, '../tsconfig.json'),
                            outFile: '',
                            transpileOnly: true,
                            silent: process.env.NODE_ENV === 'dependencies'
                        })}`
                    ]
                },
            ]
        }]
    },
    plugins: [
        new webpack.DefinePlugin({
            __disableLogging__: false,
            __disableIframeDetection__: false,
            __disableNetworkDetection__: false,
            __disableTypeDetection__: false,
            __disableCookiematching__: false,
            __hermione__: false,
            __test__: false
        }),
        new webpack.DefinePlugin({
            __bundle__: '"library"',
            __dev__: false,
            __protocol__: 'https:'
        }),
        uglifyJs
    ]
};
