local function get_request_info ()
    if ngx.var.unistat_request_type == nil or ngx.var.unistat_request_type == '' then
        request_type = 'unknown'
    else
        request_type = ngx.var.unistat_request_type
    end

    request_timings = true

    return request_type, request_timings
end

local function update_metrics ()
    request_type, request_timings = get_request_info()
    status = tostring(format_status(ngx.status))
    upstream_statuses = format_upstream_statuses(ngx.var.upstream_status)
    upstream_response_times = format_upstream_timings(ngx.var.upstream_response_time)
    request_time = ngx.var.request_time

    increment_metric(string.format("prj=mds;lepton_%s_%s_dmmm", request_type, status), 1)
    if ngx.status == 406 then
        increment_metric(string.format("prj=mds;lepton_%s_406_dmmm", request_type), 1)
    end

    if tonumber(ngx.var.bytes_sent) then
        increment_metric("prj=mds;lepton_bytes_sent_dmmm", ngx.var.bytes_sent)
        increment_metric("prj=mds;lepton_bytes_sent_dmmx", ngx.var.bytes_sent)
    end

    if tonumber(ngx.var.request_length) then
        increment_metric("prj=mds;lepton_bytes_received_dmmm", ngx.var.request_length)
        increment_metric("prj=mds;lepton_bytes_received_dmmx", ngx.var.request_length)
    end

    for i, upstream_response_time in ipairs(upstream_response_times) do
        upstream_status = upstream_statuses[i]
        if upstream_status and not (string.starts(upstream_status, '5') or string.starts(upstream_status, '4')) then
            add_to_histogram(string.format("prj=mds;lepton_%s_upstream_timings_hgram", request_type), upstream_response_time)
            if tonumber(ngx.var.request_length) < 262144 and tonumber(ngx.var.bytes_sent) < 262144 then
                add_to_histogram(string.format("prj=mds;lepton_%s_upstream_250k_timings_hgram", request_type), upstream_response_time)
            end
        end
    end

    if request_time and request_time ~= '-' then
        request_time = tonumber(request_time) or 0
        if ssl_handshake_time and ssl_handshake_time ~= '-' then
            ssl_handshake_time = tonumber(ssl_handshake_time) or 0
            request_time = request_time + ssl_handshake_time
        end

        if request_timings and not (string.starts(status, '5') or string.starts(status, '4')) then
            add_to_histogram(string.format("prj=mds;lepton_%s_req_timings_hgram", request_type), request_time)
        end
        if request_timings and not (string.starts(status, '5') or string.starts(status, '4')) and
            tonumber(ngx.var.request_length) < 262144 and
            tonumber(ngx.var.bytes_sent) < 262144 then
            add_to_histogram(string.format("prj=mds;lepton_%s_req_250k_timings_hgram", request_type), request_time)
        end

    end

    increment_metric(string.format("prj=mds;lepton_%s_all_dmmm", request_type), 1)
    increment_metric('stat_module_dmmm', 1)
end

-- ngx.log(ngx.ERR, string.format('L %s', tostring(ngx.var.request_completion)))
status, err = pcall(update_metrics)
if not status then
    increment_metric('stat_module_error_dmmm', 1)
    ngx.log(ngx.ERR, string.format("Monitor status: %s. Err: %s", tostring(status), tostring(err)))
end
