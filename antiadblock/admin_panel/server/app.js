const bunyanLogger = require('express-bunyan-logger');
const compression = require('compression');
const config = require('yandex-config')();
const cookieParser = require('cookie-parser');
const Express = require('express');
const expressHttpGeobase = require('./middlewares/geobase');
const expressHttpUatraits = require('./middlewares/uatraits');
const expressHttpLangdetect = require('./middlewares/langdetect');
const expressTld = require('express-tld');
const expressYandexCsp = require('express-yandex-csp');
const expressYaFrameguard = require('./middlewares/frameguard');
const expressXff = require('express-x-forwarded-for-fix');
const expressYandexUid = require('express-yandexuid');
const helmet = require('helmet');
const router = require('./router');

const app = new Express();

const environment = require('yandex-environment');

if (environment === 'local') {
    // на дев окружении пропускаем все сертификаты
    process.env.NODE_TLS_REJECT_UNAUTHORIZED = '0';
}

app.set('env', config.environment);
app.disable('x-powered-by');
app.disable('etag');
app.enable('trust proxy');
app.use(compression());

app.use(cookieParser());
app.use(expressXff());
app.use(expressYandexCsp(config.csp));
app.use(helmet.noSniff());
app.use(helmet.xssFilter({setOnOldIE: true}));

app.use(bunyanLogger(config.logs));

if (config.render.hot) {
    app.use(require('./middlewares/app-render.hot'));
} else {
    app.use('/static', Express.static(config.statics.dir, {
        fallthrough: false
    }));
    app.use(require('./middlewares/app-render'));
}

app.use(expressTld());
app.use(expressHttpGeobase(config.geobase));
app.use(expressYaFrameguard(config.frameguard));
app.use(expressHttpUatraits(config.uatraits));
app.use(expressHttpLangdetect(config.langdetect));
app.use(expressYandexUid());

app.use((err, req, res, next) => {
    req.errorFromMiddleware = true;
    console.log(`Error from middleware: ${err}`); // eslint-disable-line no-console
    next();
});

router(app);

module.exports = app;
