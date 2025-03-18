var inherit = require('inherit'),
    BaseController = require('./base'),
    _ = require('lodash'),
    PROXY_REQUEST_HEADERS = ['accept', 'content-type', 'content-length', 'cookie', 'x-forwarded-for', 'x-forwarded-for-y', 'x-real-ip'],
    ApiController;

/** * @class
 * @extends BaseController
 */
ApiController = inherit(BaseController, /** @lends ApiController.prototype */{

    /**
     * @param {string} path
     * @private
     */
    _proxyRequest: function(path) {
        this._apiRequest(path, {
            query: this._req.query,
            headers: _.pick(this._req.headers, PROXY_REQUEST_HEADERS),
            method: this._req.method,
            body: this._req
        })
        .then(res => {
            this._onResponse(res.statusCode || 200, res.body);
        }, err => {
            this._onResponse(err.response.statusCode || 500, err.body);
        });
    },

    _onResponse(status, body) {
        this._res.status(status).json(body);
    },

    common: function() {
        // remove prefix
        var path = this._req.path.replace('/rest/', '');

        this._proxyRequest(path);
    }
});

module.exports = ApiController;
