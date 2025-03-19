-- Glue statistics for several similar buckets to single virtual bucket in charts.
--     E.g. DBAAS has lots of backup buckets with similar names. It is much more convenient
--     to have single aggregated statistics for all of this buckets (e.g. RPS, network usage and so on).
--
-- NOTE: remember you provide LUA patterns as keys, which are not pure regular expressions:
--       https://www.lua.org/pil/20.2.html
local begin_time = ngx.now()
local threshold = tonumber(ngx.var.time_threshold)
local metric_bucket_aliases = {
    ['^internal[-]dbaas']               = 'internal-dbaas',
    ['^yandexcloud[-]dbaas']            = 'yandexcloud-dbaas',
    ['^cloud[-]storage[-]']             = 'cloud-storage',
    ['^buttload']                       = 'buttload',
}

local function get_request_info ()
    -- Initialize S3 lib to make sure we have all required variables set properly
    local S3Lib = require "s3.lib"
    S3Lib.init()

    local bucket = S3Lib.Values.getBucketName()
    local external = S3Lib.Values.isPublicRequest()

    -- Show several real bucket statistics as a single virtual bucket aggregate in charts
    for pattern, alias in pairs(metric_bucket_aliases) do
        if string.match(bucket, pattern) then
            return alias, external
        end
    end

    local bucket_in_cache = ngx.shared.s3buckets:get(bucket)
    if not bucket_in_cache then
        bucket = 'unknown'
    end

    return bucket, external
end

local function get_method_type (request_method)
    local method_type
    if request_method == 'GET' then
        method_type = "read"
    elseif request_method == 'HEAD' then
        method_type = "read"
    elseif request_method == 'OPTIONS' then
        method_type = "read"
    elseif request_method == 'PUT' then
        method_type = "modify"
    elseif request_method == 'POST' then
        method_type = "modify"
    elseif request_method == 'DELETE' then
        method_type = "modify"
    elseif request_method == 'CONNECT' then
        method_type = "modify"
    else
        method_type = "unknown"
    end

    return method_type
end

local function is_listing ()
    local listing_request = false
    local delimiter = ngx.var.arg_delimiter
    local prefix = ngx.var.arg_prefix
    if delimiter or prefix then
        listing_request = true
    end

    return listing_request
end

local function unwanted_req (request_uri)
    local res = true
    if request_uri == "/" then
        return false
    end
--    local req_list = { "ping", "selfdriving", "favicon.ico", "sdc", "wal-e", "wal-g", "ch_backup", "mongodb-backup", "s3-tests-acceptance" }
    local req_list = { "ping", "favicon.ico" }
    for _,v in pairs(req_list) do
        if string.find(request_uri, v) then
            res = false
            break
        end
    end

    return res
end

-- if upstream returned 5xx or so
local function update_bucket_name_for_logs (bucket)
    local bucket_from_upstream = ngx.var.upstream_http_x_yc_s3_bucket
    if bucket_from_upstream and bucket_from_upstream ~= "" then
        ngx.var.s3_bucket_name_for_log = bucket_from_upstream
    else
        ngx.var.s3_bucket_name_for_log = bucket
    end
end

local function update_metrics ()
    local bucket, external = get_request_info()
    local request_method = ngx.var.request_method
    local method_type = get_method_type(request_method)
    local request_time = ngx.var.request_time
    local statuses = format_status_v2(ngx.status)
    local bad_status = string.starts(statuses[1], '4') or string.starts(statuses[1], '5')
    local upstream_statuses = format_upstream_statuses(ngx.var.upstream_status)
    local upstream_response_times = format_upstream_timings(ngx.var.upstream_response_time)
    local ssl_handshake_time = ngx.var.ssl_handshake_time

    local cache_status = ngx.var.upstream_cache_status
    local tags = string.format("prj=%s", bucket)
    local bytes_sent = ngx.var.bytes_sent
    local bytes_sent_num = tonumber(bytes_sent)
    local request_length = ngx.var.request_length
    local request_length_num = tonumber(request_length)
    local port = tonumber(ngx.var.server_port)
    local uri = ngx.var.request_uri

    update_bucket_name_for_logs(bucket)

    if request_method == "GET" and is_listing() then
        request_method = "LIST"
    end

    if (request_method == "GET" or request_method == "PUT") and ngx.status and ngx.status >= 200 and ngx.status < 300 and bucket == "unknown" and unwanted_req(uri) then
        increment_metric(string.format("%s;s3mds_nginx_unknown_dmmm", tags), 1, true)
--        ngx.log(ngx.ERR, string.format('L %s', tostring(ngx.var.request_completion)))
    end

    for i, status in ipairs(statuses) do
        increment_metric(string.format("%s;s3mds_nginx_%s_dmmm", tags, status), 1, true)
    end
    increment_metric(string.format("%s;s3mds_nginx_dmmm", tags), 1, true)

    if external then
        for i, status in ipairs(statuses) do
            increment_metric(string.format("%s;s3mds_nginx_external_%s_dmmm", tags, status), 1, true)
        end
        increment_metric(string.format("%s;s3mds_nginx_external_dmmm", tags), 1, true)
    end

    if bytes_sent_num then
        increment_metric(string.format("%s;s3mds_nginx_bytes_sent_dmmm", tags),
                         bytes_sent,
                         true)
        increment_metric(string.format("%s;s3mds_nginx_bytes_sent_dmmx", tags),
                         bytes_sent,
                         true)
        if external then
            increment_metric(string.format("%s;s3mds_nginx_external_bytes_sent_dmmm", tags),
                             bytes_sent,
                             true)
            increment_metric(string.format("%s;s3mds_nginx_external_bytes_sent_dmmx", tags),
                             bytes_sent,
                             true)
        end
    end
    if request_length_num then
        if ngx.var.remote_addr == "213.180.205.147" and ngx.var.server_port == "443" then
            port = 4080
        end
        increment_metric(string.format("%s;s3mds_nginx_bytes_received_dmmm", tags),
                         request_length,
                         true)
        increment_metric(string.format("%s;s3mds_nginx_port_%s_bytes_received_dmmm", tags, port),
                         request_length,
                         true)
        increment_metric(string.format("%s;s3mds_nginx_bytes_received_dmmx", tags),
                         request_length,
                         true)
        increment_metric(string.format("%s;s3mds_nginx_port_%s_bytes_received_dmmx", tags, port),
                         request_length,
                         true)
        if external then
            increment_metric(string.format("%s;s3mds_nginx_external_bytes_received_dmmm", tags),
                             request_length,
                             true)
            increment_metric(string.format("%s;s3mds_nginx_external_bytes_received_dmmx", tags),
                             request_length,
                             true)
        end
    end

    for i, status in ipairs(statuses) do
        increment_metric(string.format("%s;s3mds_nginx_%s_%s_dmmm", tags, request_method, status),
                     1,
                     true)
        if ngx.var.request_completion ~= 'OK' and status == '2xx' then
            increment_metric(string.format("%s;request_not_completed_dmmm", tags), 1, true)
        end
    end

    if request_length_num <= 5242880 and bytes_sent_num <= 5242880 then
        if request_time and request_time ~= '-' then
            request_time = tonumber(request_time) or 0
            if ssl_handshake_time and ssl_handshake_time ~= '-' then
                ssl_handshake_time = tonumber(ssl_handshake_time) or 0
                request_time = request_time + ssl_handshake_time
            end
            if not bad_status then
                add_to_histogram(string.format("%s;s3mds_nginx_%s_timings_hgram", tags, method_type),
                                 request_time,
                                 true)
            end
        end

        for i, upstream_response_time in ipairs(upstream_response_times) do
            add_to_histogram(string.format("%s;s3mds_nginx_upstream_%s_timings_hgram", tags, method_type),
                             upstream_response_time,
                             true)
        end
    end

    if cache_status and cache_status ~= '-' then
        increment_metric(string.format("%s;s3mds_nginx_cache_%s_dmmm", tags, cache_status), 1, true)
        increment_metric(string.format("%s;s3mds_nginx_cache_all_dmmm", tags), 1, true)
    end

    increment_metric('s3mds_stat_module_dmmm', 1, true)

    --
    -- Update counters of incoming client traffic
    --
    local S3Lib = require "s3.lib"
    S3Lib.countBucketWriteTraffic()

end

-- ngx.log(ngx.ERR, string.format('L %s', tostring(ngx.var.request_completion)))
local status, err = pcall(update_metrics)
if not status then
    increment_metric('s3mds_stat_module_error_dmmm', 1, true)
    ngx.log(ngx.ERR, string.format("Monitor status: %s. Err: %s", tostring(status), tostring(err)))
end

increment_metric("s3mds_stat_monitor_dmmm", 1, true)
local exec_time = ngx.now() - begin_time
if exec_time > threshold
then
    ngx.log(ngx.ERR, string.format("log_by_lua exec time: %.3f", exec_time))
end
