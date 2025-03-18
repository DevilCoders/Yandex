module.exports = function(options) {
    return function(req, res, next) {
        if ((process.env.BAYAN_BLACKBOX_MOCKS && !req.headers['accept-blackbox']) || options.mock) {
            return require('../mocks/blackbox.js')(options)(req, res, next);
        }

        return require('express-blackbox')(options)(req, res, next);
    };
};
