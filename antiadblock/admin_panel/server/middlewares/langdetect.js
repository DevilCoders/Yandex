module.exports = function(options) {
    return function(req, res, next) {
        if (!options.mock) {
            return require('express-http-langdetect')(options)(req, res, next);
        }

        req.langdetect = options.mock;

        next();
    };
};
