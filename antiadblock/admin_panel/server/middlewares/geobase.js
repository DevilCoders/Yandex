module.exports = function(options) {
    return function(req, res, next) {
        if (!options.mock) {
            return require('express-http-geobase')(options)(req, res, next);
        }

        req.geolocation = options.mock;

        next();
    };
};
