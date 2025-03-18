admin_port = admin_port or 8862;
data_port = data_port or 8863;
unistat_port = unistat_port or 8864;
ip = "localhost" or ip;
logdir = logdir or "./";
maxblob = 32768;

instance = {
    admin_addrs = {
        { ip = ip; port = admin_port; };
    };
    addrs = {
        { ip = ip; port = data_port; };
    };
    unistat = {
        addrs = {
            { ip = ip; port = unistat_port };
        };
    };
    workers = 4;
    set_no_file = false;
    log = logdir .. "/workers.log";

    -- that's a bit strange, that I have to set admin addr twice - in admin_addrs and in ipdispatch
    errorlog = {
        log = logdir .. "/error.log";
        ipdispatch = {
            control = {
                ip = ip;
                port = admin_port;
                http = {
                    maxlen = maxblob;
                    maxreq = maxblob;
                    admin = {};
                };
            };
            data = {
                ip = ip;
                port = data_port;
                http = {
                    stats_attr = "main";
                    maxlen = maxblob;
                    maxreq = maxblob;
                    accesslog = {
                        log = logdir .. "/access.log";
                        errordocument = {
                            status = 404;
                            content = "Not found.";
                        };
                    };
                };
            };
        };
    };
};
