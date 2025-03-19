local unistat_cluster_type = ngx.var.unistat_cluster_type
local request_uri = ngx.var.request_uri

--- Check if namespace is cached in nginx shared memory dict
--- @param namespace string
--- @return boolean
local function namespace_exists(namespace)
    local ns_in_cache = ngx.shared.namespaces:get(namespace)
    if ns_in_cache then
        return true
    end

    -- we don't need more than one namespace to check the cache was initialized
    local ns_names = ngx.shared.namespaces:get_keys(1)
    local cache_is_empty = table.getn(ns_names) <= 0

    if cache_is_empty then
        return true
    end

    ngx.log(ngx.ERR, string.format("Namespace %s not found in libmastermind.cache", namespace))
    return false
end

local function get_namespace(request_type)
    local unistat_namespace = ngx.var.unistat_namespace
    local namespace
    if unistat_namespace == nil or unistat_namespace == '' then
        namespace = 'unknown'
    elseif string.starts(unistat_namespace, '-') then
        namespace = 'unknown'
    else
        namespace = unistat_namespace
        if unistat_cluster_type == 'avatars' then
            if not namespace_exists(string.format("avatars-%s", namespace)) then
                namespace = 'unknown'
            end

        elseif unistat_cluster_type == 'mdsproxy' then
            if not namespace_exists(namespace) then
                namespace = 'unknown'
            end

        elseif unistat_cluster_type == 'mulcagate' then
            if namespace == 'unknown' then
                local ns = ngx.var.arg_ns
                if ns ~= nil and ns ~= ''  then
                    namespace = ns
                else
                    if string.find(request_uri, 'yadisk') or string.find(request_uri, 'docviewer') then
                        namespace = 'disk'
                    elseif string.find(request_uri, 'mail') then
                        namespace = 'mail'
                    -- request=/gate/get/103.9930.NTgxNzQxNzIuMTMvMTE5YWJhX1hYWEw
                    elseif string.find(request_uri, '%/103%.') then
                        namespace = 'default'
                    else
                        namespace = 'mail'
                    end
                end
            end

            if not namespace_exists(namespace) then
                namespace = 'unknown'
            end
        end
    end

    if request_type == 'ping' and namespace == 'unknown' then
        local user_agent = ngx.var.http_user_agent
        if user_agent ~= nil and user_agent == 'KeepAliveClient' then
            local vhost = ngx.var.host
            if vhost ~= nil and string.find(vhost, 'yandex') then
                namespace = vhost or 'unknown'
            end
        end
    end

    namespace = string.gsub(namespace, "_", "-")

    if ngx.var.unistat_namespace then
        -- Update unistat_namespace variable only when it is set in nginx configuration
        ngx.var.unistat_namespace = namespace
    end

    return namespace
end

local function get_request_info ()
    local unistat_request_type = ngx.var.unistat_request_type
    local request_type
    if unistat_cluster_type == 'resizer' then
        if unistat_request_type == nil or unistat_request_type == '' then
            if string.sub(request_uri, 1, string.len('/genurl')) == '/genurl' then
                request_type = 'genurl'
            else
                request_type = 'any'
            end
        else
            request_type = unistat_request_type
        end
    elseif unistat_cluster_type == 'mulcagate' then
        if unistat_request_type == nil or unistat_request_type == '' then
            if string.find(request_uri, '/get/') then
                request_type = 'get'
            elseif string.find(request_uri, '/put/') then
                request_type = 'put'
            else
                request_type = 'unknown'
            end
        else
            request_type = unistat_request_type
        end
    else
        if unistat_request_type == nil or unistat_request_type == '' then
            request_type = 'unknown'
        else
            request_type = unistat_request_type
        end

    end

    local namespace = get_namespace(request_type)

    return namespace, request_type
end

local function get_user_info (namespace)
    local user
    if unistat_cluster_type == 'avatars' then
        if namespace == 'images-thumbs' or namespace == 'images-taas-consumers' then
            local ref = ngx.var.arg_ref
            if ref ~= nil and ref ~= ''  then
                user = ref
            end
        end
    end
    return user
end

local function update_metrics ()
    if unistat_cluster_type == 'avatars' and ngx.var.host == 'avatars-regional.storage.yandex.net' then
        increment_metric('stat_system_dmmm', 1)
        return
    end
    if request_uri == '/hostlist' then
        increment_metric('stat_system_dmmm', 1)
        return
    end
    local namespace, request_type = get_request_info()
    namespace = string.gsub(namespace, "^-", "_")

    local tags = string.format("prj=%s", namespace)

    if unistat_cluster_type == 'avatars' then
        if ngx.var.http_x_yandex_l7 == 'yes' then
            tags = tags .. ';' .. 'tier=l7'
        elseif ngx.var.http_y_service == 'avatars_mds_yandex_net' then
            tags = tags .. ';' .. 'tier=avatarsmdsl7'
        end
        local unistat_mds_cluster = ngx.var.unistat_mds_cluster
        if unistat_mds_cluster ~= nil and unistat_mds_cluster ~= '' then
            tags = tags .. ';' .. 'mds_cluster=' .. unistat_mds_cluster
        end
    end

    local user = get_user_info(namespace)
    if user ~= nil and user ~= '' then
        tags = tags .. ';' .. 'user=' .. user
    end

    -- MDS-8345
    local unistat_prior = ngx.var.unistat_prior
    if unistat_prior ~= nil and unistat_prior ~= '' then
        tags = tags .. ';' .. 'prior=' .. unistat_prior
    end

    -- MDSSUPPORT-1064
    local unistat_service = ngx.var.unistat_service
    if unistat_service ~= nil and unistat_service ~= '' then
        tags = tags .. ';' .. 'mds_service=' .. unistat_service
    end

    local statuses = format_status_v2(ngx.status)
    local bad_status = string.starts(statuses[1], '4') or string.starts(statuses[1], '5')
    local upstream_statuses = format_upstream_statuses(ngx.var.upstream_status)
    local upstream_response_times = format_upstream_timings(ngx.var.upstream_response_time)
    local upstream_connect_times = format_upstream_timings(ngx.var.upstream_connect_time)
    local upstream_header_times = format_upstream_timings(ngx.var.upstream_header_time)
    local upstream_queue_times = format_upstream_timings(ngx.var.upstream_queue_time)
    local request_time = ngx.var.request_time
    local ssl_handshake_time = ngx.var.ssl_handshake_time

    for i, status in ipairs(statuses) do
        increment_metric(string.format("%s;namespace_%s_%s_dmmm", tags, request_type, status), 1)
        if ngx.var.request_completion ~= 'OK' and status == '2xx' then
            increment_metric(string.format("%s;request_not_completed_dmmm", tags), 1)
        end
    end

    -- MDS-7628
    local x_mds_err = ngx.header['x-mds-err']
    if x_mds_err ~= nil and x_mds_err == 'Pessimistic-5xx' then
        increment_metric(string.format("%s;namespace_%s_pessimistic_5xx_dmmm", tags, request_type), 1)
    end

    local bytes_sent = ngx.var.bytes_sent
    local bytes_sent_num = tonumber(bytes_sent)
    if bytes_sent_num then
        increment_metric(string.format("%s;namespace_bytes_sent_dmmm", tags), bytes_sent)
        increment_metric(string.format("%s;namespace_bytes_sent_dmmx", tags), bytes_sent)
    end

    local request_length = ngx.var.request_length
    local request_length_num = tonumber(request_length)
    if request_length_num then
        increment_metric(string.format("%s;namespace_bytes_received_dmmm", tags), request_length)
        increment_metric(string.format("%s;namespace_bytes_received_dmmx", tags), request_length)
    end

    for i, upstream_status in ipairs(upstream_statuses) do
        increment_metric(string.format("%s;namespace_%s_upstream_%s_dmmm", tags, request_type, upstream_status), 1)
    end

    for i, upstream_response_time in ipairs(upstream_response_times) do
        local upstream_status = upstream_statuses[i]
        if upstream_status and not (string.starts(upstream_status, '5') or string.starts(upstream_status, '4')) then
            add_to_histogram(string.format("%s;namespace_%s_upstream_timings_hgram", tags, request_type), upstream_response_time)
            if request_length_num < 5242880 and bytes_sent_num < 5242880 then
                add_to_histogram(string.format("%s;namespace_%s_upstream_5mb_timings_hgram", tags, request_type), upstream_response_time)
            end
        end
    end
    for i, upstream_connect_time in ipairs(upstream_connect_times) do
        local upstream_status = upstream_statuses[i]
        if upstream_status and not (string.starts(upstream_status, '5') or string.starts(upstream_status, '4')) then
            add_to_histogram(string.format("%s;namespace_%s_upstream_connect_timings_hgram", tags, request_type), upstream_connect_time)
        end
    end
    for i, upstream_header_time in ipairs(upstream_header_times) do
        local upstream_status = upstream_statuses[i]
        if upstream_status and not (string.starts(upstream_status, '5') or string.starts(upstream_status, '4')) then
            add_to_histogram(string.format("%s;namespace_%s_upstream_header_timings_hgram", tags, request_type), upstream_header_time)
        end
    end
    for i, upstream_queue_time in ipairs(upstream_queue_times) do
        local upstream_status = upstream_statuses[i]
        if upstream_status and not (string.starts(upstream_status, '5') or string.starts(upstream_status, '4')) then
            add_to_histogram(string.format("%s;namespace_%s_upstream_queue_timings_hgram", tags, request_type), upstream_queue_time)
        end
    end

    local cache_status = ngx.var.upstream_cache_status
    if cache_status and not (cache_status == '-') then
        increment_metric(string.format("%s;namespace_%s_cache_%s_dmmm", tags, request_type, cache_status), 1)
        increment_metric(string.format("%s;namespace_%s_cache_all_dmmm", tags, request_type), 1)

        if cache_status == 'HIT' then
            add_to_histogram(string.format("%s;namespace_%s_cache_timings_hgram", tags, request_type), request_time)
        end
    end

    if request_time and request_time ~= '-' then
        request_time = tonumber(request_time) or 0
        if ssl_handshake_time and ssl_handshake_time ~= '-' then
            ssl_handshake_time = tonumber(ssl_handshake_time) or 0
            request_time = request_time + ssl_handshake_time
        end

        if not bad_status then
            add_to_histogram(string.format("%s;namespace_%s_req_timings_hgram", tags, request_type), request_time)
            if request_length_num < 5242880 and bytes_sent_num < 5242880 then
                add_to_histogram(string.format("%s;namespace_%s_req_5mb_timings_hgram", tags, request_type), request_time)
            end
        end

    end

    increment_metric(string.format("%s;namespace_%s_all_dmmm", tags, request_type), 1)
    increment_metric('stat_module_dmmm', 1)
end

-- ngx.log(ngx.ERR, string.format('L %s', tostring(ngx.var.request_completion)))
local status, err = pcall(update_metrics)
if not status then
    increment_metric('stat_module_error_dmmm', 1)
    ngx.log(ngx.ERR, string.format("Monitor status: %s. Err: %s", tostring(status), tostring(err)))
end
