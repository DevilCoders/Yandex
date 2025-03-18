require('babel-register')();
const webpack = require('webpack');
const _ = require('lodash');
const webpackConfig = require('../../webpack/client.config');

const devMiddleware = require('webpack-dev-middleware');
const hotMiddleware = require('webpack-hot-middleware');
const ssr = require('../../app/ssr.jsx').default;

function createMiddleware(lang) {
    const langConfig = webpackConfig.byLangs[lang];
    const compiler = webpack(langConfig);
    const hMiddleware = hotMiddleware(compiler);
    const dMiddleware = devMiddleware(compiler, {
        publicPath: langConfig.output.publicPath,
        historyApiFallback: true,
        stats: {
            colors: true
        }
    });

    return (req, res, next) => {
        if (_.get(req, 'langdetect.id', 'ru') !== lang) {
            next();
            return;
        }
        dMiddleware(req, res, function() {
            hMiddleware(req, res, function() {
                next();
            });
        });
    };
}

module.exports = Object.keys(webpackConfig.byLangs).map(createMiddleware).concat(
    (req, res, next) => {
        res.render = ssr;
        next();
    }
);
