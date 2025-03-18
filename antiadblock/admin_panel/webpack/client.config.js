const path = require('path');
const webpack = require('webpack');
const ExtractTextPlugin = require('extract-text-webpack-plugin');
const CopyWebpackPlugin = require('copy-webpack-plugin');

const config = require('yandex-config')();

const env = process.env.NODE_ENV;
const IS_PRODUCTION = (env === 'production');
const IS_LOCAL = (env === 'local');

let stylesLoader = [
    'style-loader',
    'css-loader',
    {
        loader: 'postcss-loader',
        options: {
            plugins: () => [require('postcss-cssnext')]
        }
    }
];

// if production wrap styles in ExtractTextPlugin
if (IS_PRODUCTION) {
    const fallback = stylesLoader.shift();
    stylesLoader = ExtractTextPlugin.extract({
        fallback,
        use: stylesLoader
    });
}

const BUNDLES = [
    'antiadb',
    'error'
];

const vendorLibs = [
    'babel-polyfill',
    'react',
    'react-dom',
    'redux',
    'react-redux',
    'react-helmet',
    'react-router-dom',
    'whatwg-fetch'
];

const langs = Object.keys(config.build.languages).filter(lang => config.build.languages[lang]);

function getBuildPath(bundle) {
    const prodPath = path.join(__dirname, `../app/bundles/${bundle}/entry`);
    const devPath = [
        'react-hot-loader/patch',
        'webpack-hot-middleware/client',
        prodPath
    ];

    return IS_PRODUCTION ? prodPath : devPath;
}

function createLangConfig(lang) {
    const entry = {};

    BUNDLES.forEach(bundle => {
        entry[`${bundle}.${lang}`] = getBuildPath(bundle);
    });

    entry.vendor = vendorLibs;

    const langConfig = {
        name: 'client',
        entry: entry,
        output: {
            path: path.join(__dirname, '../static/build'),
            filename: '[name].js',
            libraryTarget: 'this',
            library: '__init__',
            devtoolModuleFilenameTemplate: '/[resource-path]'
        },
        resolve: {
            extensions: ['.js', '.jsx'],
            alias: {
                app: path.resolve(__dirname, '../app/')
            }
        },
        module: {
            rules: [
                {
                    test: /\.jsx?$/,
                    loader: 'babel-loader',
                    options: {
                        presets: ['es2015', 'stage-3', 'react'],
                        plugins: ['react-hot-loader/babel'],
                        ignore: /node_modules/,
                        babelrc: false,
                        cacheDirectory: IS_LOCAL
                    }
                },
                {
                    test: /\.css$/,
                    loader: stylesLoader
                },
                {
                    test: /\.(jpg|gif|png|eot|otf|woff2?|ttf)$/,
                    loader: 'file-loader'
                },
                {
                    test: /\.svg/,
                    use: [
                        {
                            loader: 'svg-url-loader',
                            options: {
                                dataUrlLimit: 1024
                            }
                        },
                        {
                            loader: 'svgo-loader',
                            options: {
                                multipass: true,
                                plugins: [
                                    {removeDesc: true},
                                    {removeTitle: true},
                                    {sortAttrs: true},
                                    {removeViewBox: true},
                                    {removeStyleElement: true}
                                ]
                            }
                        }
                    ]
                }
            ]
        },
        plugins: [
            new webpack.optimize.ModuleConcatenationPlugin(),
            new webpack.HotModuleReplacementPlugin(),
            new webpack.optimize.CommonsChunkPlugin({
                name: 'vendor',
                filename: 'vendor.js',
                minChunks: function (module) {
                    if (module.resource && (/^.*\.css$/).test(module.resource)) {
                        return false;
                    }
                    return module.context && module.context.indexOf('node_modules') !== -1;
                }
            }),
            new ExtractTextPlugin('[name].build.css'),
            new webpack.NoEmitOnErrorsPlugin(),
            new webpack.DefinePlugin({
                'process.env.NODE_ENV': JSON.stringify(env),
                'process.env.BEM_LANG': JSON.stringify(lang)
            }),
            new CopyWebpackPlugin([
                {
                    from: 'node_modules/monaco-editor/min/vs',
                    to: 'vs'
                }
            ])
        ],
        target: 'web',
        devtool: IS_PRODUCTION ? 'source-maps' : 'eval-source-map'
    };

    if (IS_PRODUCTION) {
        langConfig.plugins.push(new webpack.optimize.UglifyJsPlugin({
            // TODO: не забыть отцепить!
            compress: {
                warnings: false
            },
            sourceMap: true
        }));
    } else {
        langConfig.output.publicPath = '/static/build/';
    }

    return langConfig;
}

const langConfigs = langs.map(createLangConfig);

module.exports = {
    buildConfigs: langConfigs,
    byLangs: langs.reduce((map, lang, index) => {
        map[lang] = langConfigs[index];
        return map;
    }, {})
};
