var environment = process.env.NODE_ENV || require('yandex-environment'),
    _ = require('lodash'),
    options = {
        timestamp: true,
        handleExceptions: true
    };

if (environment !== 'local') {
    _.extend(options, {
        json: true,
        stringify: true
    });
}

module.exports = options;
