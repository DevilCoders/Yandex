console.log(`AntiAdBlock is building for ${process.env.NODE_ENV}`); // eslint-disable-line no-console
const clientConfig = require('./client.config');
const serverConfig = require('./server.config');

module.exports = [serverConfig].concat(clientConfig.buildConfigs);
