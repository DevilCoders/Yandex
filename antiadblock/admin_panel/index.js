const config = require('yandex-config')();
const app = require('./server');

app.listen(config.server.port, () => {
    console.log(`Listen on http://${config.server.host}:${config.server.port}`); // eslint-disable-line no-console
});
