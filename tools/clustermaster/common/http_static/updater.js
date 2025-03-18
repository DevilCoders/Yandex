function BaseUpdater() {
    this.handlerIsSet = function() {
        return (typeof this.handler === 'function');
    };

    this.setHandler = function(h) {
        this.handler = h;
    };

    this.hold = function() {
        if ('timer' in this) {
            clearTimeout(this.timer);
            delete this.timer;
        }
    };

    this.serv = function(url, revision) {
        this.hold();

        this.request = new XMLHttpRequest();
        this.request.open('GET', url + (url.match(/\?[^/\\]*$/) ? '&r=' : '?r=') + (revision || 0), true);

        var requestHandler = function(updater, url, revision) {
            return function() {
                if (this.readyState == 4) {
                    if (this.status == 200) try {
                        var object = eval('(' + this.responseText + ')');

                        revision = object['#'];
                        delete object['#'];

                        if (updater.handlerIsSet()) {
                            try {
                                updater.handler(object);
                            } catch (e) {}
                        }
                    } catch (e) {}

                    delete updater.request;

                    if (Env.UpdaterDelay) {
                        (function(updater, url, revision) {
                            updater.timer = setTimeout(function() {
                                updater.serv(url, revision)
                            }, Env.UpdaterDelay);
                        })(updater, url, revision);
                    }
                }
            };
        }(this, url, revision);
        this.request.onreadystatechange = requestHandler;

        this.request.send(null);
    };
};

function TableUpdater() {
    this.baseUpdater = new BaseUpdater();
    this.handlers = {};

    this.register = function(i, h) {
        this.handlers[i] = h;
    };

    this.registerAfter = function(f) {
        this.afterHandler = f;
    };

    this.hold = function() {
        this.baseUpdater.hold();
    };

    this.serv = function(url, revision) {
        this.baseUpdater.serv(url, revision);
    };

    var baseUpdaterHandler = function(tableUpdater) {
        return function(object) {
            for (var i in object) {
                if (i in tableUpdater.handlers) try {
                    tableUpdater.handlers[i](object[i]);
                } catch (e) {}
            }
            if (typeof tableUpdater.afterHandler === 'function') try {
                tableUpdater.afterHandler(object);
            } catch (e) {}
        };
    }(this);
    this.baseUpdater.setHandler(baseUpdaterHandler);
};

Updater = new TableUpdater();