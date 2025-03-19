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

local function format_status (status)
    if status == nil or status == '-' then
        return status
    else
        for x in string.gmatch(status, "%d+") do
            if tonumber(x) then
                status = tonumber(x)
            end
        end
    end

    if not tonumber(status) then
        return '-'
    end
    
    if status >= 200 and status < 300 then
        return '2xx'
    elseif status >= 300 and status < 400 then
        return '3xx'
    elseif status == 403 then
        return '403'
    elseif status == 404 then
        return '404'
    elseif status == 429 then
        return '429'
    elseif status >= 400 and status < 500 then
        return '4xx'
    elseif status >= 500 then
        return '5xx'
    else
        return '1xx'
    end

end

local function capitalize(first, rest)
    return first:upper()..rest:lower()
end

local function update_metrics ()
   local status = format_status(ngx.status)
   local host = ngx.var.host
   local method_type = ngx.var.request_method
   local request_time = format_timings(ngx.var.request_time)
   local upstream_response_time = format_timings(ngx.var.upstream_response_time)
   local cache_status = ngx.var.upstream_cache_status

   if ngx.var.request_uri == '/ping' then
       return
   end

   local tags = {}
   tags.nginxhost = host
   tags.status = status
   tags.method = method_type
   tags.cache_status = cache_status

   tags.name = 'rps'
   tags.proxy_service = 'nginx'
   increment_solomon_metric(tags, 1)

   local tags = {}
   tags.nginxhost = host
   tags.proxy_service = 'nginx'
   tags.name = 'bytes'
   if tonumber(ngx.var.bytes_sent) then
       tags.traffic_type = 'sent'
       increment_solomon_metric(tags, ngx.var.bytes_sent)
   end
   if tonumber(ngx.var.request_length) then
       tags.traffic_type = 'receive'
       increment_solomon_metric(tags, ngx.var.request_length)
   end

   local tags = {}
   tags.nginxhost = host
   tags.proxy_service = 'nginx'
   tags.name = "timings"
   tags.method_type = method_type
   if tonumber(ngx.var.request_length) <= 10485760 and tonumber(ngx.var.bytes_sent) <= 10485760 then
      if request_time and request_time ~= '-' then
          if status ~= '4xx' and status ~= '403' and status ~= '404 'and status ~= '5xx' then
              tags.upstream = "no"
              add_to_solomon_histogram(tags, request_time)
         end
      end

      if upstream_response_time and not (upstream_response_time == '-') then
          tags.upstream = "yes"
          add_to_solomon_histogram(tags, upstream_response_time)
      end
   end

   local service_tags = {}
   service_tags.name = 'lua_stat'
   service_tags.request_stat_type = 'module'
   increment_solomon_metric(service_tags, 1)
end

status, err = pcall(update_metrics)
if not status then
    local service_tags = {}
    service_tags.name = 'lua_stat'
    service_tags.request_stat_type = 'module_error'
    increment_solomon_metric(service_tags, 1)
end
