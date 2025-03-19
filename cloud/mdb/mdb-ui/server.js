const express = require('express')
const proxy = require('express-http-proxy')
const fs = require('fs');
const morgan = require('morgan');
const cors = require('cors');

const app = express();

const port = process.env.PORT || 3001;
const token = process.env.TOKEN || "";

app.use(morgan('tiny'));
app.use(cors());
const HOSTS = {
    'porto-test': 'deploy-api-test.db.yandex-team.ru',
    'porto-prod': 'deploy-api.db.yandex-team.ru',
}
app.use('/', proxy(HOSTS['porto-prod'], {
    https: true,
    filter: function(req, res) {
        return req.method == 'GET';
    },
    proxyReqOptDecorator: function (proxyReqOpts, srcReq) {
        if (token !== "") {
            proxyReqOpts.headers['Authorization'] = 'OAuth ' + token;
        }
        proxyReqOpts.rejectUnauthorized = false;
        return proxyReqOpts
    },
}));

app.listen(port, function () {
    console.log("Server started on " + port + "!");
});
