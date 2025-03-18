const config = require('yandex-config')();

const controllers = {
    base: require('./controllers/base'),
    antiadb: require('./controllers/antiadb'),
    api: require('./controllers/api')
};

module.exports = function(app) {
    var permissions = config.permissions, // eslint-disable-line no-unused-vars
        routes = {
            '/': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE]
                }
            },

            '/ping': {
                action: 'antiadb.succ200',
                options: {
                    needUserCheck: false,
                    permissions: []
                }
            },

            '/logs/view/res_id/:resId/type/:type/req_id/:reqId': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE]
                }
            },

            '/state/:deviceType?': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE]
                }
            },

            '/search': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE]
                }
            },
            '/heatmap': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.SERVICE_CREATE]
                }
            },

            '/support': {
                action: 'antiadb.index',
                options: {
                    permissions: []
                }
            },

            '/service': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_CREATE]
                }
            },

            '/service/:serviceId': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/screenshots-checks/diff/:leftRunId/:rightRunId': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/:action': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/label/:labelId': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/label/:labelId/configs/:configId/status/*?': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/configs/diff/:configId/:secondConfigId': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/sbs-profiles/diff/:profileId/:secondProfileId': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/service/:serviceId/sbs-result-checks*': {
                action: 'antiadb.index',
                options: {
                    permissions: [permissions.INTERFACE_SEE, permissions.SERVICE_SEE]
                }
            },

            '/rest/*': {
                action: 'api.common',
                method: 'all',
                options: {
                    permissions: []
                }
            },

            '*': {
                action: 'antiadb.error404',
                options: {
                    needUserCheck: false,
                    permissions: []
                }
            }
        };

    Object.keys(routes).forEach(function(route) {
        var routeSettings, controllerName, method, httpMethod,
            Controller, options, action, parts;

        routeSettings = routes[route];

        if (typeof routeSettings === 'object') {
            method = routeSettings.action;
            options = routeSettings.options;
        } else if (typeof routeSettings === 'string') {
            method = routeSettings;
        }

        parts = method.split('.');
        controllerName = parts[0];
        action = parts[1];
        Controller = controllers[controllerName];

        if (typeof Controller !== 'function') {
            throw new Error('Undefined controller "' + controllerName + '"');
        }

        httpMethod = routeSettings.method || 'get';

        app[httpMethod](route, function(req, res) {
            var controller = new Controller(req, res, options);

            controller.processAction(action);
        });
    });
};
