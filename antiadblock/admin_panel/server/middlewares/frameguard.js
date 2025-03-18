const {URL} = require('url');

module.exports = function(options) {
    return function(req, res, next) {
        try { // если пустой referer, то парсер URL падает
            const host = new URL(req.headers.referer).hostname;
            const isAllowedDomain = Boolean(options.whitelist.find(pattern => host.endsWith(pattern)));

            if (isAllowedDomain) {
                return next();
            }
        } catch (e) {}

        return require('express-ya-frameguard')(options)(req, res, next);
    };
};
