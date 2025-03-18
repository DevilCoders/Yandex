module.exports = function(options) {
    return function(req, res, next) {
        if (!options.mock) {
            return require('express-uatraits-mock')(options)(req, res, next);
        }

        req.uatraits = options.mock;

        next();
    };
};
