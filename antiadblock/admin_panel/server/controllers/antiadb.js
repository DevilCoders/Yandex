const config = require('yandex-config')();
const inherit = require('inherit');

const BaseController = require('./base');

const _ = require('lodash');

const AntiadbController = inherit(BaseController, {

    index: function() {
        return this.requestRender();
    },

    _getData: function() {
        return _.extend(this.__base(), {
            nonce: this._req.nonce,
            staticHost: config.statics.host,
            baseUrl: config.app.baseUrl,
            apiUrl: config.api.url,
            apiVersion: config.api.version,
            permissions: config.permissions,
            bundle: 'antiadb'
        });
    }
});

module.exports = AntiadbController;
