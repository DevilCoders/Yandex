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

local function get_request_info ()
    bucket = ''

    hostnames = {'storage.yandexcloud.net',
                 'website.yandexcloud.net',
                 'storage.cloud-preprod.yandex.net',
                 'website.cloud-preprod.yandex.net',
                 's3-yc-test.yandex.net'}

    for key, host in ipairs(hostnames) do
        if host == ngx.var.host then
            bucket = string.match(ngx.var.request_uri, "^/+([^/?]+)")
        end
    end

    if bucket == '' then
        for key, host in ipairs(hostnames) do
            if string.find(ngx.var.host, host) then
                bucket = string.gsub(ngx.var.host, '.' .. host .. '$', '')
            end
        end
    end

    if bucket == '' then
       bucket = "nosuchbucket"
    end

    if ngx.var.request_method == 'GET' then
       method_type = "read"
    elseif ngx.var.request_method == 'HEAD' then
       method_type = "read"
    elseif ngx.var.request_method == 'PUT' then
       method_type = "modify"
    elseif ngx.var.request_method == 'POST' then
       method_type = "modify"
    elseif ngx.var.request_method == 'DELETE' then
       method_type = "modify"
    else
       method_type = "unknown"
    end

    return bucket, method_type
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

local function empty_metrics (bucket)
    local methods = {"GET", "HEAD", "POST", "PUT", "DELETE"}
    local status = {"2xx", "3xx", "4xx", "403", "404", "5xx", "501"}
    local upstreams = {"yes", "no"}

    local tags = {}
    tags.bucket = bucket
    for idx_m = 1, #methods do
        for idx_s = 1, #status do
            tags.method = methods[idx_m]
            tags.status = status[idx_s]
            increment_solomon_metric(tags, 0)
        end
    end
end

local function capitalize(first, rest)
    return first:upper()..rest:lower()
end

local function prepeare_aws_compatible(name)
    return string.gsub(name, "(%a)([%w_']*)", capitalize).."Requests"
end

local function update_metrics ()
   local bucket, method_type = get_request_info()
   local status = format_status(ngx.status, false)
   local request_time = format_timings(ngx.var.request_time)
   local upstream_response_time = format_timings(ngx.var.upstream_response_time)
   local cache_status = ngx.var.upstream_cache_status

   if not ngx.resp.get_headers()["x-yc-s3-folder-id"] then
       bucket = 'nosuchbucket'
   end

   empty_metrics(bucket)

   local tags = {}
   tags.bucket = bucket
   tags.status = status
   tags.method = ngx.var.request_method
   tags.cached_request = "no"

   if cache_status and cache_status ~= '-' then
       tags.cache_status = cache_status
       tags.cached_request = "yes"
   end

   increment_solomon_metric(tags, 1)

   if ngx.resp.get_headers()["x-yc-s3-folder-id"] then
       local tags_client = {}
       tags_client.bucket = bucket
       tags_client.name = "method"
       tags_client.cloud_id = ngx.resp.get_headers()["x-yc-s3-cloud-id"]
       tags_client.folder_id = ngx.resp.get_headers()["x-yc-s3-folder-id"]
       tags_client.method = prepeare_aws_compatible(ngx.var.request_method)

       increment_solomon_client_metric(tags_client, 1)
   end


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
