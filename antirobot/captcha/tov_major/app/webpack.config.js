const { merge } = require('webpack-merge');
const webpack = require('webpack');

const common = require('./webpack.common.js');

module.exports = merge(common, {
    mode: 'dev',
    optimization: {
        minimize: true,
    },
    output: {
        filename: "tmgrdfrend-debug.js"
    },
    plugins: [
        new webpack.DefinePlugin({
            'process.env.NODE_ENV': '"dev"',
        }),
    ],
});
