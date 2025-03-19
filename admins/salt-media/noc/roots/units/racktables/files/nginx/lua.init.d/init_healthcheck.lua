local hc = require "healthcheck"

local check_primary = {
    http_req = "GET /export/slb-check.php?q=ismaster&origin=lua-upstream-healthcheck HTTP/1.0\r\nHost: racktables.yandex-team.ru\r\n\r\n",
    valid_statuses = {200},
    valid_reply = "1",
}
local check_backup = {
    http_req = "GET /export/slb-check.php?q=ro&origin=lua-upstream-healthcheck HTTP/1.0\r\nHost: ro.racktables.yandex-team.ru\r\n\r\n",
    valid_statuses = {200},
}

local ok, err = hc.spawn_checker{
    shm = "healthcheck",  -- defined by "lua_shared_dict"
    upstream = "rt_http", -- upstream with check points
    type = "http",
    check_primary = check_primary,
    check_backup = check_backup,
    port = 80,

    interval = 2000,  -- run the check cycle every 2 sec
    timeout = 1000,   -- 1 sec is the timeout for network operations
    fall = 3,  -- # of successive failures before turning a peer down
    rise = 2,  -- # of successive successes before turning a peer up
    concurrency = 10,  -- concurrency level for test requests
}
if not ok then
    ngx.log(ngx.ERR, "failed to spawn health checker: ", err)
    return
end

local ok, err = hc.spawn_checker{
    shm = "healthcheck",  -- defined by "lua_shared_dict"
    upstream = "rt_ro_http", -- upstream with check points
    type = "http",
    check_primary = check_backup,
    port = 80,
    interval = 2000,  -- run the check cycle every 2 sec
    timeout = 1000,   -- 1 sec is the timeout for network operations
    fall = 3,  -- # of successive failures before turning a peer down
    rise = 2,  -- # of successive successes before turning a peer up
    concurrency = 10,  -- concurrency level for test requests
}

if not ok then
    ngx.log(ngx.ERR, "failed to spawn health checker: ", err)
    return
end
