var winston = require('winston'),
    options = require('../../configs/logger'),
    logger;

logger = new (winston.Logger)({
    transports: [
        new (winston.transports.Console)(options)
    ]
});

module.exports = logger;
