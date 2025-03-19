local function get_mds_service(namespace)
    local mds_service

    if namespace ~= nil then
        -- MDS-14404
        if namespace == 'music' then
            local user_agent = ngx.var.http_user_agent
            if user_agent and (string.starts(user_agent, 'YandexStation') or string.starts(user_agent, 'yandexmini')) then
                for u in string.gmatch(user_agent, "[^/]+") do
                    mds_service = u
                    break
                end
            end
        end
    end

    return mds_service
end

--- Get namespace name from nginx variable
--- @return string
local function get_namespace()
    local namespace = ngx.var.unistat_namespace
    if namespace == nil then
        namespace = "none"
    end
    if namespace == "" then
        namespace = "EMPTY"
    end

    if ngx.var.unistat_namespace then
        -- Update unistat_namespace variable only when it is set in nginx configuration
        ngx.var.unistat_namespace = namespace
    end

    return namespace
end

--- Get backend name from nginx variable
--- @return string
local function get_backend()
    local backend = ngx.var.unistat_backend
    if backend == "" then
        backend = "UNKNOWN"
    end

    return backend
end

--- Get request type from nginx variable
--- @return string
local function get_request_type()
    local unistat_request_type = ngx.var.unistat_request_type

    if unistat_request_type == nil or unistat_request_type == '' then
        return 'get'
    end

    return unistat_request_type
end

local function get_request_info ()
    local request_type = get_request_type()
    local namespace = get_namespace()
    local backend = get_backend()

    local cloud_mimic_addr = ngx.var.cloud_mimic_addr
    if cloud_mimic_addr == nil then
        cloud_mimic_addr = '2a02:6b8:0:3400:0:37f:0:3'
    end

    local upstream
    local upstream_addr = ngx.var.upstream_addr
    if upstream_addr ~= nil then
        if string.find(upstream_addr, cloud_mimic_addr) then
            upstream = 'upstream-cloudmimic'
        elseif string.find(upstream_addr, '127.0.0.1:11000') or string.find(upstream_addr, '%[::1%]:11000') then
            upstream = 'upstream-mimic'
        elseif upstream_addr == 'unix:/var/run/mediastorage/mediastorage-proxy.sock' then
            upstream = 'upstream-mediastorage'
        else
            ngx.log(ngx.ERR, "upstream-other: ", upstream_addr)
            upstream = "upstream-other"
        end
    else
        upstream = backend
    end

    return namespace, upstream, request_type
end

local function update_tag_metrics (tags, request_type)

    local statuses = format_status_v2(ngx.status)
    local bad_status = string.starts(statuses[1], '4') or string.starts(statuses[1], '5')
    local upstream_statuses = format_upstream_statuses(ngx.var.upstream_status)
    local upstream_response_times = format_upstream_timings(ngx.var.upstream_response_time)
    local request_time = ngx.var.request_time
    local ssl_handshake_time = ngx.var.ssl_handshake_time

    for i, status in ipairs(statuses) do
        increment_metric(string.format("%s;%s_%s_dmmm", tags, request_type, status), 1)
        if ngx.var.request_completion ~= 'OK' and status == '2xx' then
            increment_metric(string.format("%s;request_not_completed_dmmm", tags), 1)
        end
    end

    local bytes_sent = ngx.var.bytes_sent
    local bytes_sent_num = tonumber(bytes_sent)
    if bytes_sent_num then
        increment_metric(string.format("%s;bytes_sent_dmmm", tags), bytes_sent)
        increment_metric(string.format("%s;bytes_sent_dmmx", tags), bytes_sent)
    end

    local request_length = ngx.var.request_length
    local request_length_num = tonumber(request_length)
    if request_length_num then
        increment_metric(string.format("%s;bytes_received_dmmm", tags), request_length)
        increment_metric(string.format("%s;bytes_received_dmmx", tags), request_length)
    end

    for i, upstream_status in ipairs(upstream_statuses) do
        increment_metric(string.format("%s;%s_upstream_%s_dmmm", tags, request_type, upstream_status), 1)
    end

    for i, upstream_response_time in ipairs(upstream_response_times) do
        local upstream_status = upstream_statuses[i]
        if upstream_status and not (string.starts(upstream_status, '5') or string.starts(upstream_status, '4')) then
            add_to_histogram(string.format("%s;%s_upstream_timings_hgram", tags, request_type), upstream_response_time)
            if request_length_num < 5242880 and bytes_sent_num < 5242880 then
                add_to_histogram(string.format("%s;%s_upstream_5mb_timings_hgram", tags, request_type), upstream_response_time)
            end
        end
    end

    local cache_status = ngx.var.upstream_cache_status
    if cache_status and not (cache_status == '-') then
        increment_metric(string.format("%s;%s_cache_%s_dmmm", tags, request_type, cache_status), 1)
        increment_metric(string.format("%s;%s_cache_all_dmmm", tags, request_type), 1)

        if cache_status == 'HIT' then
            add_to_histogram(string.format("%s;%s_cache_timings_hgram", tags, request_type), request_time)
        end
    end

    if request_time and not (request_time == '-') then
        if ssl_handshake_time and not (ssl_handshake_time == '-') then
            ssl_handshake_time = tonumber(ssl_handshake_time) or 0
            request_time = tonumber(request_time) or 0
            request_time = request_time + ssl_handshake_time
        end

        if not bad_status then
            add_to_histogram(string.format("%s;%s_req_timings_hgram", tags, request_type), request_time)
        end
    end

    increment_metric(string.format("%s;%s_all_dmmm", tags, request_type), 1)
    increment_metric('stat_module_dmmm', 1)
end

local function update_metrics()
    local namespace, upstream, request_type = get_request_info()

    local tags = string.format("prj=%s", namespace)
    if upstream ~= nil then
        tags = string.format("%s;tier=%s", tags, upstream)
    end

    local mds_service = get_mds_service(namespace)
    if mds_service ~= nil and mds_service ~= '' then
        tags = string.format("%s;mds_service=%s", tags, mds_service)
    end

    update_tag_metrics(tags, request_type)
end

local status, err = pcall(update_metrics)
if not status then
    increment_metric('stat_module_error_dmmm', 1)
    ngx.log(ngx.ERR, string.format("Monitor status: %s. Err: %s. Upstream_addr: %s.", tostring(status), tostring(err), tostring(ngx.var.upstream_addr)))
end
