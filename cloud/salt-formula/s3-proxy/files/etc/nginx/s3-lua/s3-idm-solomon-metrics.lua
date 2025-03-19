local function format_timings (t)
    if t == nil or t == '-' then
        return t
    else
        local time = 0.000
        for x in string.gmatch(t, "%d.%d%d%d") do
            if tonumber(x) then
                s = tonumber(x)
                time = time + s
            end
        end
        return time
    end
end

local function get_method_type (method)
    if method == 'GET' then
       method_type = "read"
    elseif method == 'HEAD' then
       method_type = "read"
    elseif method == 'PUT' then
       method_type = "modify"
    elseif method == 'POST' then
       method_type = "modify"
    elseif method == 'DELETE' then
       method_type = "modify"
    else
       method_type = "unknown"
    end
    return method_type
end

local function get_request_info ()
    location = string.match(ngx.var.request_uri, "^/+([^/?]+)")

    if location ~= 'stats' and location ~= 'management' then
       location = "unknown"
    end

    method_type = get_method_type(ngx.var.request_method)

    return location, method_type
end

local function format_status (s, upstream)
    if upstream then
        if s == nil or s == '-' then
            return s
        else
            for x in string.gmatch(s, "%d+") do
                if tonumber(x) then
                    s = tonumber(x)
                end
            end
        end

        if not tonumber(s) then
            return '-'
        end
    end

    if s >= 200 and s < 300 then
        return '2xx'
    elseif s >= 300 and s < 400 then
       return '3xx'
    elseif s == 403 then
       return '403'
    elseif s == 404 then
       return '404'
    elseif s >= 400 and s < 500 then
       return '4xx'
    elseif s == 501 then
       return '501'
    elseif s >= 500 then
       return '5xx'
    else
       return '1xx'
    end

end

local function empty_metrics (location)
    local methods = {"GET", "HEAD", "POST", "PUT", "DELETE"}
    local status = {"2xx", "3xx", "4xx", "403", "404", "5xx", "501"}
    local upstreams = {"yes", "no"}

    local tags = {}
    tags.location = location
    for idx_m = 1, #methods do
        for idx_s = 1, #status do
            tags.method = methods[idx_m]
            tags.status = status[idx_s]
            tags.method_type = get_method_type(methods[idx_m])
            increment_solomon_metric(tags, 0)
        end
    end
end

local function update_metrics ()

    local location, method_type = get_request_info()
    local status = format_status(ngx.status, false)
    local request_time = format_timings(ngx.var.request_time)
    local upstream_response_time = format_timings(ngx.var.upstream_response_time)

    empty_metrics(location)

    local tags = {}
    tags.location = location
    tags.status = status
    tags.method = ngx.var.request_method
    tags.method_type = method_type

    increment_solomon_metric(tags, 1)

    if tonumber(ngx.var.bytes_sent) then
        tags.traffic_type = "sent"
        increment_solomon_metric(tags, ngx.var.bytes_sent)
    end
    if tonumber(ngx.var.request_length) then
        tags.traffic_type = "receive"
        increment_solomon_metric(tags, ngx.var.request_length)
    end

    if tonumber(ngx.var.request_length) <= 5242880 and tonumber(ngx.var.bytes_sent) <= 5242880 then
        if request_time and request_time ~= '-' then
            if status ~= '4xx' and status ~= '403' and status ~= '404 'and status ~= '5xx' then
                tags.upstream = "no"
                tags.method_type = method_type
                add_to_solomon_histogram(tags, request_time)
            end
        end

        if upstream_response_time and not (upstream_response_time == '-') then
            tags.upstream = "yes"
            tags.method_type = method_type
            add_to_solomon_histogram(tags, upstream_response_time)
        end
    end

    local service_tags = {}
    service_tags.request_stat_type = "module"
    increment_solomon_metric(service_tags, 1)
end

status, err = pcall(update_metrics)
if not status then
    local service_tags = {}
    service_tags.request_stat_type = "module_error"
    increment_solomon_metric(service_tags, 1)
end
