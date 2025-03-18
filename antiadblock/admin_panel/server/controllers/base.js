var inherit = require('inherit'),
    vowGot = require('vow-got'),
    vow = require('vow'),
    config = require('yandex-config')(),
    _ = require('lodash'),
    Url = require('url'),
    logger = require('../lib/logger'),
    winston = require('winston'),
    querystring = require('querystring'),
    BaseController;

/**
 * @class
 */
BaseController = inherit(/** @lends BaseController.prototype */{

    /**
     * @param {Object} req
     * @param {Object} res
     * @param {Object} [options]
     * @protected
     */
    __constructor: function(req, res, options) {
        options = options || {};

        this._req = req;

        this._res = res;

        this._data = this._getData();

        this._needRender = true;
        this._needRedirect = false;
        this._needUserCheck = typeof options.needUserCheck !== 'undefined' ? options.needUserCheck : true;

        this._requiredPermissions = _.union(this._getRoutePermissions(options));

        this._requestBaseUrl = Url.parse(config.api.url);
        this._requestVersion = config.api.version;

        this._requestDefaults = {
            json: true,
            timeout: config.connections.apiRequestTimeout,
            headers: {
                'accept-encoding': 'gzip, deflate',
                connection: 'keep-alive'
            }
        };
    },

    /**
     * @param {Object} options
     * @returns {Array}
     * @protected
     */
    _getRoutePermissions: function(options) {
        return typeof options.permissions !== 'undefined' ?
            options.permissions :
            [];
    },

    /**
     * @returns {Object}
     * @protected
     */
    _getData: function() {
        return {
            version: config.app.version,
            stHost: config.statics.host,
            host: this._req.hostname,
            lang: _.get(this._req, 'langdetect.id', 'ru'),
            contentRegion: _.get(this._req, 'tld', '').split('.').pop(),
            tld: this._req.tld,

            yandexuid: _.get(this._req.cookies, 'yandexuid')
        };
    },

    /**
     * @param {string} action
	 * @param {Object} options
     */
    processAction: function(action, options) {
		options = options || {};

        if (typeof this[action] !== 'function') {
            throw new Error('Undefined action "' + action + '"');
        }

        this._data.action = action;

        if (this._req.errorFromMiddleware) {
            return this.error500();
        }

        this._checkUser()
            .then(function() {
                this._processAction(action, options);
            }, this)
            .catch(this.requestError, this);
    },

    /**
     * @param {string} action
     * @param {Object} options
     * @private
     */
    _processAction: function(action, options) {
        this[action](options);
    },

    /**
     * @public
     */
    error404: function() {
        this.requestError(this._getErrorObject(404));
    },

    /**
     * @public
     */
    error500: function() {
        return this.requestError({});
    },

    /**
     * @public
     */
    succ200: function() {
        this._res.sendStatus(200);
    },

    /**
     * @returns {Vow.Promise}
     * @private
     */
    _checkUser: function() {
        var promise,
            dfd = vow.defer(),
            serviceId = this._req.params.serviceId,
            permissionsUrl = serviceId ? `auth/permissions/service/${serviceId}` : 'auth/permissions/global',
            servicesUrl = 'services',
            PROXY_REQUEST_HEADERS = ['cookie', 'x-forwarded-for', 'x-forwarded-for-y', 'x-real-ip'],
            headers = {
                'accept-language': this._req.langdetect.id,
                ..._.pick(this._req.headers, PROXY_REQUEST_HEADERS)
            };

        // TODO
        // У нас нет отдельных прав на доступ к админке -> проверяем,
        // что у пользователя не пустой список сервисов и интерпретируем это как permissions.INTERFACE_SEE
        // В идеале нужно добавить на это отдельные права
        promise = vow.all([
                this._apiRequest(permissionsUrl, {
                    headers: headers
                }),
                this._apiRequest(servicesUrl, {
                    headers: headers
                })
            ])
            .then(function([permissionsResponse, servicesResponse]) {
                var user = permissionsResponse.body,
                    permissions = _.get(user, 'permissions', []),
                    services = _.get(servicesResponse, 'body.items', []);

                if (permissions.indexOf(config.permissions.SERVICE_CREATE) !== -1 || services.length) {
                    _.set(user, 'permissions', permissions.concat(config.permissions.INTERFACE_SEE));
                }

                this._data.user = user;
                if (this._needUserCheck && !this._checkAccess(user)) {
                    logger.warn('Permissions problem. Required permission: %j; user: %j.',
                        this._requiredPermissions,
                        user);
                    dfd.reject(this._getErrorObject(403));
                } else {
                    dfd.resolve(permissionsResponse);
                }

                return dfd.promise();
            }, function(error) {
                dfd.reject(this._getErrorObject(!error.code || error.code === 500 ? 500 : 403));

                return dfd.promise();
            }, this);

        return promise;
    },

    /**
     * @returns {boolean}
     * @protected
     */
    _checkAccess: function(user) {
        return user.user_id && Array.isArray(user.permissions) &&
            _.isEqual(this._requiredPermissions, _.intersection(this._requiredPermissions, user.permissions));
    },

    /**
     * @returns {boolean}
     * @protected
     */
    _hasServices: function(services) {
        return _.get(services, 'total', 0) > 0;
    },

    /**
     * @returns {string}
     * @protected
     */
    _getRestrictionReasonI18N: function() {
        return 'not-allowed';
    },

    /**
     * @returns {string}
     * @protected
     */
    _getNotFoundReasonI18N: function() {
        return 'not-found';
    },

    /**
     * @returns {Object}
     * @protected
     */
    _getErrorObject: function(errorCode) {
        var errorMessageI18N;

        switch (errorCode) {
            case 403:
                errorMessageI18N = this._getRestrictionReasonI18N();
                break;
            case 404:
                errorMessageI18N = this._getNotFoundReasonI18N();
                break;
            default:
                errorMessageI18N = 'default';
        }

        return {
            response: {
                statusCode: errorCode
            },
            body: {
                messageI18N: errorMessageI18N
            }
        };
    },

    /**
     * @public
     */
    requestRender: function() {
        if (!this._needRender) {
            return;
        }

        this._render();
    },

    /**
     * @param {Object} err
     */
    requestError: function(err) {
        var response = _.extend({
                status: (err.response && err.response.statusCode) || 500,
                message: err.body && err.body.message
            }, err.body);

        if (this._req.xhr) {
            this._data.json = response;
        } else {
            this._data.pageView = 'error';
            this._data.error = response;
        }

        if (err instanceof Error) {
            logger.error(winston.exception.getAllInfo(err));
        }

        this._render();
    },

    /**
     * @private
     */
    _render: function() {
        var pageView = this._data.pageView,
            json = this._data.json,
            error = this._data.error;

        if (this._req.xhr) {
            this._res.json(json.status || 200, json);
            return;
        }

        this._res.status(error ? error.status : 200);
        try {
            if (error) {
                this._data.bundle = 'error';
            }
            this._res.render(this._data.bundle, this._req.url, this._data)
                .then(({redirectUrl, html}) => {
                    if (redirectUrl) {
                        this._res.redirect(redirectUrl);
                    } else {
                        this._res.send(html);
                    }
                }, this.requestError, this);
        } catch (e) {
            if (pageView !== 'error') {
                this.requestError(e);
            }
        }
    },

    /**
     * @param {string} url
     * @param {Object} params
     * @returns {Array}
     * @protected
     */
    _getApiRequestParams: function(url, params) {
        var requestURL = _.extend({}, this._requestBaseUrl, {
                pathname: this._requestVersion + '/' + url
            }),
            requestParams = _.merge({}, this._requestDefaults, params, {
                headers: {
                    authorization: _.get(this._req.headers, 'authorization', null),
                    'accept-language': this._req.langdetect.id
                }
            });

        return [Url.format(requestURL), requestParams];
    },

    /**
     * @param {string} url
     * @param {Object} [params]
     * @returns {Vow.Promise}
     * @protected
     */
    _apiRequest: function(url, params) {
        var requestParams = this._getApiRequestParams(url, params),
            promise = vowGot.apply(vowGot, requestParams);

        promise.catch(function(error) {
            this._logError(requestParams[0], requestParams[1], error);
        }, this);

        return promise;
    },

    /**
     *
     * @param {string} path
     * @param {Object} options
     * @param {Error} error
     */
    _logError: function(path, options, error) {
        logger.error('backend api error. Path: %s, method: %s, error %j',
            path + '?' + querystring.stringify(options.query),
            options.method || 'GET', {
                body: error.body,
                status: error.code,
                text: error.toString()
            }, {});
    }

});

module.exports = BaseController;
