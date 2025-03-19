--- S3Lib is a Yandex S3 API request processing library for Nginx.
--- It contains common request processing functions that can't be implemented
--- with regular nginx config but are required since we can't/don't want to move
--- the logic to Goose.
---   Examples:
---     - limit overall bucket write/read request rate with YARL (more reliable way of DoS protection)
---     - limit overall bucket in/out traffic with YARL (it is much easier in nginx than in Go)
---     - perform internal redirect to specific location (nginx just doesn't provide it out of the box)
---     - ...
---
local S3Internal = require "s3.internal"
local S3Log = require "s3.log"
local S3Values = require "s3.values"
local S3Lock = require "s3.lock"
local S3OpenBuckets = require "s3.open_buckets"

local LOCKS_DICT_NAME = "s3_locks"
S3Internal.require_dict(LOCKS_DICT_NAME)

local S3Lib = {}
S3Lib.Values = S3Values
S3Lib.Logger = S3Log

-- 'require "yarl"' here breaks S3Lib import in init_by_lua*.
-- YARL lib can be imported only after worker fork (init_worker_by_lua*).
-- It just silently doesn't work otherwise because of GO runtime get broken by forks
--local YARL = require "yarl/yarl-go"

--- Initialize worker processes and variables.
--- Should be called once in init_worker_by_lua.
function S3Lib.initWorker()
    local ok, err = ngx.timer.at(0, S3Lib.updateOpenBuckets, 120, 600)
    if not ok then
        S3Internal.break_nginx("failed to start 'updateOpenBuckets' periodic task: " .. tostring(err))
    end
end

--- Initialize variables for current request.
--- Should be called at least once for each request
--- Subsequent calls inside single request are safe and do not cause variables rewrite.
--- Always returns '1' string and can be used in set_by_lua* directives, e.g.:
---   set_by_lua_block $s3_lib_initialized {
---         local S3Lib = require "s3.lib"
---         return S3Lib.init()
---   }
--- @return string
function S3Lib.init()
    if S3Values.isInitialized() then
        return
    end

    local bucket, external = S3Internal.getBucketName()

    S3Values.setBucketName(bucket)
    S3Values.setPublicRequest(external)

    S3Values.setInitialized()
    return "1"
end

--- Perform redirect to proper location according to S3Lib configuration.
function S3Lib.route()
    -- Here you can define routes for buckets with custom nginx settings
    -- local bucket = S3Values.getBucketName()

    -- if bucket == nil or bucket == "" then
    --     return ngx.exit(ngx.HTTP_FORBIDDEN)
    -- end

    -- local location = "@some_bucket_location"

    -- S3Log.Info("Routing request to location '" .. location .. "'")
    -- return ngx.exec(location)
end

function S3Lib.limitBucketRate()
    local bucket_name = S3Values.getBucketName()

    if bucket_name == "" then
        return
    end

    local YARL = require 'yarl/yarl-go'
    local yarl_env = S3Values.getYarlEnv()

    local priority = ""
    if S3Values.isPrioritizedRequest() then
        priority = "high"
    end

    if S3Values.isReadRequest() then

        local rate_quota = S3Internal.getYarlQuotaName(
                yarl_env, "r", bucket_name, "read", priority
        )

        S3Log.Info("YARL.limit_by_unique_name('" .. rate_quota .. "', 1)")
        YARL.check_by_unique_name(rate_quota, 1)

        local traffic_quota = S3Internal.getYarlQuotaName(
                yarl_env, "t", bucket_name, "read", ""
        )
        S3Log.Info("YARL.check_by_unique_name('" .. traffic_quota .. "')")
        YARL.check_by_unique_name(traffic_quota)

    elseif S3Values.isWriteRequest() then

        local rate_quota = S3Internal.getYarlQuotaName(
                yarl_env, "r", bucket_name, "write", priority
        )
        S3Log.Info("YARL.limit_by_unique_name('" .. rate_quota .. "', 1)")
        YARL.check_by_unique_name(rate_quota, 1)

        local port = S3Values.getServerPort()
        if port == 80 or port == 443 then
            local traffic_quota = S3Internal.getYarlQuotaName(
                    yarl_env, "t", bucket_name, "write", ""
            )
            S3Log.Info("YARL.check_by_unique_name('" .. traffic_quota .. "')")
            YARL.check_by_unique_name(traffic_quota)
        end

    end
end

--- This method should be executed in body_filter_by_lua_*
function S3Lib.countBucketReadTraffic()
    local bucket_name = S3Values.getBucketName()

    if not S3Values.isReadRequest()
        or bucket_name == "" then
        return
    end

    local YARL = require 'yarl/yarl-go'
    local yarl_env = S3Values.getYarlEnv()
    local response_length = string.len(ngx.arg[1])

    local traffic_quota = S3Internal.getYarlQuotaName(
            yarl_env, "t", bucket_name, "read", ""
    )
    local kbytes_sent = math.ceil(response_length / 1024)

    S3Log.Info("YARL.increment_by_unique_name('" .. traffic_quota .. "', " .. tostring(kbytes_sent) .. ")")
    YARL.increment_by_unique_name(traffic_quota, kbytes_sent)
end

--- This method should be executed in log_by_lua_*
function S3Lib.countBucketWriteTraffic()
    local bucket_name       = S3Values.getBucketName()
    local request_length    = S3Values.getRequestLength()
    local port              = S3Values.getServerPort()

    if not S3Values.isWriteRequest()
        or bucket_name == ""
        or request_length == 0
        or (port ~= 80 and port ~= 443) then
        return
    end

    local YARL = require 'yarl/yarl-go'
    local yarl_env = S3Values.getYarlEnv()

    local traffic_quota = S3Internal.getYarlQuotaName(
        yarl_env, "t", bucket_name, "write", ""
    )
    local kbytes_received = math.ceil(request_length / 1024)

    S3Log.Info("YARL.increment_by_unique_name('" .. traffic_quota .. "', " .. tostring(kbytes_received) .. ")")
    YARL.increment_by_unique_name(traffic_quota, kbytes_received)
end

-- TODO: replace this patchy logic with custom bucket locations with their own settings. This will be both faster and more readable.
function S3Lib.fillHeaders()
    local cfg_x_robots_tag              = "common"
    local cfg_service_worker_allowed    = "common"
    local cfg_x_frame_options           = "common"
    local cfg_cors_settings             = "common"

    local cfg_cors_settings_map = {
        ["^vh[-]districts[-]test[-]converted$"]         = "strm",
        ["^vh[-]districts[-]unstable[-]converted$"]     = "strm",
        ["^internal[-]vh[-][%a%d_-]+[-]converted$"]     = "strm",
        ["^stream[-]ad$"]                               = "strm",
        ["^strm[-]vast$"]                               = "strm",
        ["^transcoder[-]test$"]                         = "strm",
    }

    local bucket_name = S3Values.getBucketName()

    --
    -- Select configurations for buckets
    --
    if S3Values.isPublicRequest() then
        -- Public accessible buckets (e.g. <bucket>.s3.yandex.net)
        if bucket_name == "games" then
            cfg_service_worker_allowed = 'games'
        end

        if string.match(bucket_name, "hotosho") then
            cfg_x_frame_options = 'allow-all'
        end

        if string.match(bucket_name, "ott[-]static")
            or string.match(bucket_name, "robots[-]files")
            or string.match(bucket_name, "s3[-]dopomoga")
        then
            cfg_x_robots_tag = "no_tags"
        end
    else
        -- Yandex internal buckets (e.g. <bucket>.s3.mds.yandex.net)
        for pattern, cors_settings in pairs(cfg_cors_settings_map) do
            if string.match(bucket_name, pattern) then
                cfg_cors_settings = cors_settings
                break
            end
        end
    end

    --
    -- Set response headers for selected configurations
    --
    if cfg_cors_settings == "common" then
        ngx.header.Access_Control_Allow_Origin = "*"
    elseif cfg_cors_settings == "strm" then
        ngx.header.Access_Control_Allow_Origin = ngx.var.http_origin
        ngx.header.Access_Control_Allow_Credentials = "true"
    end

    if cfg_service_worker_allowed == "games" then
        ngx.header.Service_Worker_Allowed = "/"
        ngx.header.Referrer_Policy = "no-referrer-when-downgrade"
        -- ngx.header.Content_Security_Policy_Report_Only = "default-src 'self'; report-uri https://csp.yandex.net/csp?from=games-game&project=games&yandex_login=&yandexuid=;"
        ngx.header.Content_Security_Policy_Report_Only = "child-src 'self' blob: mc.yandex.ru; connect-src 'self' localhost.msup.yandex.ru mc.yandex.ru amc.yandex.ru an.yandex.ru jstracer.yandex.ru verify.yandex.ru *.verify.yandex.ru csp.yandex.net strm.yandex.ru strm.yandex.net *.strm.yandex.net favicon.yandex.net avatars.mds.yandex.net yandexmetrica.com www.google-analytics.com www.googletagmanager.com games-sdk.yandex.az games-sdk.yandex.by games-sdk.yandex.co.il games-sdk.yandex.com games-sdk.yandex.com.am games-sdk.yandex.com.ge games-sdk.yandex.com.tr games-sdk.yandex.ee games-sdk.yandex.fr games-sdk.yandex.kg games-sdk.yandex.kz games-sdk.yandex.lt games-sdk.yandex.lv games-sdk.yandex.md games-sdk.yandex.ru games-sdk.yandex.tj games-sdk.yandex.tm games-sdk.yandex.ua games-sdk.yandex.uz; default-src 'self'; font-src 'self' yastatic.net yastat.net an.yandex.ru fonts.gstatic.com; frame-src 'self' localhost yastatic.net; img-src 'self' data: blob: mc.yandex.ru amc.yandex.ru an.yandex.ru verify.yandex.ru *.verify.yandex.ru favicon.yandex.net avatars.mds.yandex.net games.games-test.yandex.ru games-sdk.yandex.az games-sdk.yandex.by games-sdk.yandex.co.il games-sdk.yandex.com games-sdk.yandex.com.am games-sdk.yandex.com.ge games-sdk.yandex.com.tr games-sdk.yandex.ee games-sdk.yandex.fr games-sdk.yandex.kg games-sdk.yandex.kz games-sdk.yandex.lt games-sdk.yandex.lv games-sdk.yandex.md games-sdk.yandex.ru games-sdk.yandex.tj games-sdk.yandex.tm games-sdk.yandex.ua games-sdk.yandex.uz; media-src 'self' data: blob: strm.yandex.ru *.strm.yandex.net; script-src 'self' 'unsafe-eval' 'unsafe-inline' blob: yandex.ru *.yandex.ru yastatic.net; style-src 'self' 'unsafe-eval' 'unsafe-inline' fonts.googleapis.com; report-uri https://csp.yandex.net/csp?from=games-game&project=games&yandex_login=&yandexuid=;"
    end

    if cfg_x_frame_options == "allow-all" then
        ngx.header.X_Frame_Options = "Allow-From *"
    end

    if cfg_x_robots_tag == "common" then
        ngx.header.X_Robots_Tag = "noindex, noarchive, nofollow"
    end

    if ngx.status == 204 then
        ngx.header.Content_Type = ""
    end
end

--
-- External buckets access control
--

function S3Lib.updateOpenBuckets(premature, refresh_time, in_memory_ttl)
    if premature then
        S3Log.Info("stopping 'updateOpenBuckets' periodic run because of worker shutdown")
        return
    end

    local ok, err = ngx.timer.at(refresh_time, S3Lib.updateOpenBuckets, refresh_time, in_memory_ttl)
    if not ok then
        S3Log.Error("failed to schedule next 'updateOpenBuckets' run: " .. tostring(err))
    end

    local lock

    lock, err = S3Lock:new(LOCKS_DICT_NAME, "open_bucket_settings", "worker PID " .. tostring(ngx.worker.pid()))
    if err then
        S3Log.Error("failed to initialize lock object: " .. tostring(err))
        return
    end

    local locked

    locked, err = lock:try_lock(refresh_time)
    if not locked then
        S3Log.Info("failed to get lock for 'open_bucket_settings' action: " .. tostring(err))
        return
    end

    S3OpenBuckets.refresh(in_memory_ttl)
    lock:unlock()
end

function S3Lib.checkPublicAccessAllowed(bucket_name)
    -- Allow only READ requests to public buckets
    if ngx.var.request_method ~= "GET" and
            ngx.var.request_method ~= "HEAD" and
            ngx.var.request_method ~= "OPTIONS" then
        ngx.exit(ngx.HTTP_NOT_ALLOWED)
    end

    if not bucket_name then
        bucket_name = S3Values.getBucketName()
    end

    if not S3OpenBuckets.isPublicAccessAllowed(bucket_name) then
        ngx.exit(ngx.HTTP_FORBIDDEN)
    end
end

function S3Lib.checkPrivateAccessAllowed(bucket_name)
    if not bucket_name then
        bucket_name = S3Values.getBucketName()
    end

    if not S3OpenBuckets.isPrivateAccessAllowed(bucket_name) then
        ngx.exit(ngx.HTTP_FORBIDDEN)
    end
end

function S3Lib.checkStaffAccessAllowed(bucket_name)
    if not bucket_name then
        bucket_name = S3Values.getBucketName()
    end

    if not S3OpenBuckets.isStaffAccessAllowed(bucket_name) then
        ngx.exit(ngx.HTTP_FORBIDDEN)
    end
end

function S3Lib.checkZenAccessAllowed(bucket_name)
    if not bucket_name then
        bucket_name = S3Values.getBucketName()
    end

    if not S3OpenBuckets.isZenAccessAllowed(bucket_name) then
        ngx.exit(ngx.HTTP_FORBIDDEN)
    end
end

return S3Lib
