if cjson == nil then
    cjson = require "cjson"
end

if lfs == nil then
    lfs = require "lfs"
end

local function try_run(module, method)
    local status, lib = pcall(require, module)
    if not status then
        return nil
    end

    local result
    status, result = pcall(lib[method])
    if not status then
        return nil
    end

    return result
end

try_run("s3.lib", "initWorker")

local HISTOGRAM_LOG_BASE = 1.35
local HISTOGRAM_MIN_LOG = -23
local HISTOGRAM_MAX_LOG = 26
local HISTOGRAM_BORDERS = {0}
for idx = HISTOGRAM_MIN_LOG, HISTOGRAM_MAX_LOG do
    HISTOGRAM_BORDERS[idx-HISTOGRAM_MIN_LOG+2] = HISTOGRAM_LOG_BASE ^ idx
end
-- this is current histogram buckets, they represent timings in range from 1ms to 2500s
-- for user-buckets no more than 50 buckets acceptable
-- for i in `seq -23 26` ; do echo 1.35^$i | bc -l ; done
-- .00100536398756688296
-- .00135724138321529200
-- .00183227586734064420
-- .00247357242090986967
-- .00333932276822832406
-- .00450808573710823748
-- .00608591574509612060
-- .00821598625587976281
-- .01109158144543767980
-- .01497363495134086773
-- .02021440718431017143
-- .02728944969881873144
-- .03684075709340528744
-- .04973502207609713805
-- .06714227980273113637
-- .09064207773368703410
-- .12236680494047749603
-- .16519518666964461964
-- .22301350200402023652
-- .30106822770542731930
-- .40644210740232688106
-- .54869684499314128943
-- .74074074074074074074
-- 1
-- 1.35
-- 1.8225
-- 2.460375
-- 3.32150625
-- 4.4840334375
-- 6.053445140625
-- 8.17215093984375
-- 11.0324037687890625
-- 14.893745087865234375
-- 20.10655586861806640625
-- 27.14385042263438964843
-- 36.64419807055642602539
-- 49.46966739525117513427
-- 66.78405098358908643127
-- 90.15846882784526668222
-- 121.71393291759111002099
-- 164.31380943874799852834
-- 221.82364274230979801326
-- 299.46191770211822731791
-- 404.27358889785960687918
-- 545.76934501211046928689
-- 736.78861576634913353730
-- 994.66463128457133027536
-- 1342.79725223417129587174
-- 1812.77629051613124942685
-- 2447.24799219677718672625

local TEMP_HISTOGRAM_VALUES = {}
local TEMP_HISTOGRAM_NAMES = {}
local TEMP_NUMERIC_METRICS = {}

local EXT_TEMP_HISTOGRAM_VALUES = {}
local EXT_TEMP_HISTOGRAM_NAMES = {}
local EXT_TEMP_NUMERIC_METRICS = {}

local TRANSFER_DELAY = 1.0
local NAMESPACES_TRANSFER_DELAY = 60.0
local UNISTAT_CRIT_TIME = 1.0
local UNISTAT_WARN_TIME = 0.5
local MIN_FLUSH_DELAY = 300
local AUTO_FLUSH_DELAY = 43200


local function check_error(key, ok, err)
    if not ok then
        ngx.log(ngx.ERR, "Failed to update metric key " .. key .. ": " .. err)
        return false
    end
    return true
end

local function increment_shared_dict_value(dict, key, value)
    local newval, err = dict:incr(key, value)
    if not newval and err == "not found" then
        if check_error(key, dict:safe_add(key, 0)) then
            dict:incr(key, value)
        end
    end
end

local function increment_dict_value(dict, key, value)
    if dict[key] == nil then
        dict[key] = value
    else
        dict[key] = dict[key] + value
    end
end

local function transfer_metrics()
    if not ngx.worker.exiting() then
        local temp_table

        temp_table, TEMP_NUMERIC_METRICS = TEMP_NUMERIC_METRICS, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.numeric_metrics, key, value)
        end
        temp_table, TEMP_HISTOGRAM_VALUES = TEMP_HISTOGRAM_VALUES, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.histogram_values, key, value)
        end
        temp_table, TEMP_HISTOGRAM_NAMES = TEMP_HISTOGRAM_NAMES, {}
        for key in pairs(temp_table) do
            check_error(name, ngx.shared.histogram_names:safe_set(key, 1))
        end

        temp_table, EXT_TEMP_NUMERIC_METRICS = EXT_TEMP_NUMERIC_METRICS, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.ext_numeric_metrics, key, value)
        end
        temp_table, EXT_TEMP_HISTOGRAM_VALUES = EXT_TEMP_HISTOGRAM_VALUES, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.ext_histogram_values, key, value)
        end
        temp_table, EXT_TEMP_HISTOGRAM_NAMES = EXT_TEMP_HISTOGRAM_NAMES, {}
        for key in pairs(temp_table) do
            check_error(name, ngx.shared.ext_histogram_names:safe_set(key, 1))
        end

        local ok, err = ngx.timer.at(TRANSFER_DELAY + math.random(0, 25) / 100.0 * TRANSFER_DELAY, transfer_metrics)
        if not ok then
             ngx.log(ngx.ERR, "failed to create the timer transfer_metrics: ", err)
        end
    end
end

local function get_namespaces()
    if not ngx.worker.exiting() then
        if ngx.config.ngx_lua_version > 9017 then
            local worker = ngx.worker.id()
            if worker == 0 then
                local begin = ngx.now()
                local file = io.open("/var/tmp/mm.namespaces")
                local counter = 0
                if file then
                    for line in file:lines() do
                        ngx.shared.namespaces:safe_set(line, true)
                        counter = counter + 1
                    end
                end
                io.close(file)
                local duration = string.format("%.3f", ngx.now() - begin)
                ngx.log(
                    ngx.ERR,
                    duration .. " sec. Found " .. counter .. " namespaces in /var/tmp/mm.namespaces"
                )
            end
        else
            ngx.log(ngx.ERR, "Nginx lua version " .. ngx.config.ngx_lua_version .. " <= 9.0.17. Skip namespaces check")
        end

        local ok, err = ngx.timer.at(NAMESPACES_TRANSFER_DELAY, get_namespaces)
        if not ok then
             ngx.log(ngx.ERR, "failed to create the timer get_namespaces: ", err)
        end
    end
end

local function get_s3buckets()
    if not ngx.worker.exiting() then
        if ngx.config.ngx_lua_version > 9017 then
            local worker = ngx.worker.id()
            if worker == 0 then
                local begin = ngx.now()
                local size = lfs.attributes("/var/cache/yasm/buckets_names", "size")
                local file = io.open("/var/cache/yasm/buckets_names")
                local counter = 0
                if file then
                    for line in file:lines() do
                        ngx.shared.s3buckets:safe_set(line, true)
                        counter = counter + 1
                        if counter % 1000 == 0 then
                            ngx.sleep(0.002)
                        end
                    end
                end
                io.close(file)
                local duration = string.format("%.3f", ngx.now() - begin)
                ngx.log(
                    ngx.ERR,
                    duration .. " sec. Found " .. counter .. " S3 buckets in /var/cache/yasm/buckets_names with size " .. size .. ";"
                )
            end
        else
            ngx.log(ngx.ERR, "Nginx lua version " .. ngx.config.ngx_lua_version .. " <= 9.0.17. Skip S3 buckets check")
        end

        local ok, err = ngx.timer.at(NAMESPACES_TRANSFER_DELAY, get_s3buckets)
        if not ok then
             ngx.log(ngx.ERR, "failed to create the timer get_s3buckets: ", err)
        end
    end
end

local function auto_flush()
    if not ngx.worker.exiting() then
        local worker = ngx.worker.id()
        if worker == 0 then
            flush(true, true)
        end

        local ok, err = ngx.timer.at(AUTO_FLUSH_DELAY * math.random(100, 200) / 100.0, auto_flush)
        if not ok then
             ngx.log(ngx.ERR, "failed to create the timer auto_flush: ", err)
        end
    end
end

-- TODO: https://github.com/openresty/lua-nginx-module#ngxtimerevery for v0.10.9
local ok, err = ngx.timer.at(0, get_namespaces)
if not ok then
     ngx.log(ngx.ERR, "failed to create the timer get_namespaces: ", err)
end

local ok, err = ngx.timer.at(0, get_s3buckets)
if not ok then
     ngx.log(ngx.ERR, "failed to create the timer get_s3buckets: ", err)
end

local ok, err = ngx.timer.at(TRANSFER_DELAY, transfer_metrics)
if not ok then
     ngx.log(ngx.ERR, "failed to create the timer transfer_metrics: ", err)
end

local ok, err = ngx.timer.at(AUTO_FLUSH_DELAY * math.random(100, 200) / 100.0, auto_flush)
if not ok then
     ngx.log(ngx.ERR, "failed to create the timer auto_flush: ", err)
end

function increment_metric(name, value, ext)
    if value == nil then
        ngx.log(ngx.ERR, "Trying to set nil value for metric " .. name)
    else
        if ext == true then
            increment_dict_value(EXT_TEMP_NUMERIC_METRICS, name, value)
        else
            increment_dict_value(TEMP_NUMERIC_METRICS, name, value)
        end
    end
end


function add_to_histogram(name, value, ext)
    local offset = 0
    if value == nil then
        ngx.log(ngx.ERR, "Trying to add nil value to histogram for metric " .. name)
        return
    else
        offset = math.floor(math.max(HISTOGRAM_MIN_LOG - 1, math.min(HISTOGRAM_MAX_LOG, math.log(value) / math.log(HISTOGRAM_LOG_BASE)))) - HISTOGRAM_MIN_LOG + 2
    end

    if ext == true then
        increment_dict_value(EXT_TEMP_HISTOGRAM_VALUES, name .. offset, 1)
        EXT_TEMP_HISTOGRAM_NAMES[name] = 1
    else
        increment_dict_value(TEMP_HISTOGRAM_VALUES, name .. offset, 1)
        TEMP_HISTOGRAM_NAMES[name] = 1
    end
end


local function yasmify_histogram(name, ext)
    local hvalues
    if ext == true then
        hvalues = ngx.shared.ext_histogram_values
    else
        hvalues = ngx.shared.histogram_values
    end

    local output = {}
    for idx = 1, #HISTOGRAM_BORDERS do
        local value = hvalues:get(name .. idx)
        if value ~= nil then
            output[#output+1] = {HISTOGRAM_BORDERS[idx], value}
        end
    end
    return output
end


function get_yasm_metrics(ext)
    local numerics
    if ext == true then
        numerics = ngx.shared.ext_numeric_metrics:get_keys(0)
    else
        numerics = ngx.shared.numeric_metrics:get_keys(0)
    end

    local output = {}
    local value
    for idx = 1, #numerics do
        if ext == true then
            value = ngx.shared.ext_numeric_metrics:get(numerics[idx])
        else
            value = ngx.shared.numeric_metrics:get(numerics[idx])
        end
        output[#output + 1] = { numerics[idx], value }
    end

    local histograms
    if ext == true then
        histograms = ngx.shared.ext_histogram_names:get_keys(0)
    else
        histograms = ngx.shared.histogram_names:get_keys(0)
    end

    for idx = 1, #histograms do
        value = yasmify_histogram(histograms[idx], ext)
        output[#output + 1] = { histograms[idx], value }
    end

    return output
end


function flush(default, ext)
    if default == true then
        ngx.log(ngx.ERR, "Flush default metrics")
        ngx.shared.histogram_names:flush_all()
        ngx.shared.histogram_values:flush_all()
        ngx.shared.numeric_metrics:flush_all()
    end

    if ext == true then
        ngx.log(ngx.ERR, "Flush ext metrics")
        ngx.shared.ext_histogram_names:flush_all()
        ngx.shared.ext_histogram_values:flush_all()
        ngx.shared.ext_numeric_metrics:flush_all()
    end
end


function flush_metrics()
    local request_time = ngx.var.request_time
    request_time = tonumber(request_time) or 0

    local timestamp = ngx.time()
    local flush_timestamp = ngx.shared.numeric_metrics:get('flush_metrics')
    if flush_timestamp == nil or flush_timestamp <= 0 then
        flush_timestamp = timestamp
        ngx.shared.numeric_metrics:safe_set("flush_metrics", flush_timestamp)
    end

    if request_time >= UNISTAT_CRIT_TIME then
        increment_metric("stat_crit_time_dmmm", 1)
        if timestamp - flush_timestamp > MIN_FLUSH_DELAY then
            flush(true, true)
        end
    elseif request_time >= UNISTAT_WARN_TIME then
        increment_metric("stat_warn_time_dmmm", 1)
    end
end

-- Storage common functions
function format_status (s)
    if s >= 200 and s < 300 then
        return '2xx'
    elseif s >= 300 and s < 400 then
        return '3xx'
    elseif s == 404 then
        return '404'
    elseif s == 418 then
        return '418'
    elseif s == 429 then
        return '429'
    elseif s == 434 then
        return '434'
    elseif s == 499 then
        return '499'
    elseif s >= 400 and s < 500 then
        return '4xx'
    elseif s == 501 then
        return '501'
    elseif s == 507 then
        return '507'
    elseif s >= 500 then
        return '5xx'
    else
        return '1xx'
    end
end

function format_status_v2 (s)
    local statuses = {}

    if s >= 200 and s < 300 then
        table.insert(statuses, '2xx')
    elseif s >= 300 and s < 400 then
        table.insert(statuses, '3xx')
    elseif s >= 400 and s < 500 then
        table.insert(statuses, '4xx')
    elseif s >= 500 then
        table.insert(statuses, '5xx')
    else
        table.insert(statuses, '1xx')
    end

    if s == 206 then
        table.insert(statuses, '206')
    elseif s == 400 then
        table.insert(statuses, '400')
    elseif s == 401 then
        table.insert(statuses, '401')
    elseif s == 403 then
        table.insert(statuses, '403')
    elseif s == 404 then
        table.insert(statuses, '404')
    elseif s == 418 then
        table.insert(statuses, '418')
    elseif s == 429 then
        table.insert(statuses, '429')
    elseif s == 434 then
        table.insert(statuses, '434')
    elseif s == 499 then
        table.insert(statuses, '499')
    elseif s == 501 then
        table.insert(statuses, '501')
    elseif s == 507 then
        table.insert(statuses, '507')
    end

    return statuses
end

function format_upstream_statuses (s)
    local upstream_statuses = {}
    if s == nil or s == '-' then
        return {}
    else
        for st in string.gmatch(s, "%d+") do
            local st_num = tonumber(st)
            if st_num then
                local statuses = format_status_v2(st_num)
                for i, status in ipairs(statuses) do
                    table.insert(upstream_statuses, status)
                end
            end
        end
    end

    return upstream_statuses
end

function format_timings (t)
    if t == nil or t == '-' then
        return t
    else
        local time = 0.000
        for tm in string.gmatch(t, "%d+%.%d+") do
            local tm_num = tonumber(tm)
            if tm_num then
                time = time + tm_num
            end
        end
        return time
    end
end

function format_upstream_timings (t)
    local timings = {}
    if t == nil or t == '-' then
        return {}
    else
        for tm in string.gmatch(t, "%d+%.%d+") do
            local tm_num = tonumber(tm)
            if tm_num then
                local time = 0.000 + tm_num
                table.insert(timings, time)
            end
        end
    end

    return timings
end

function string.ends(String, End)
    return End == '' or string.sub(String, -string.len(End)) == End
end

function string.starts(String, Start)
   return string.sub(String, 1, string.len(Start)) == Start
end

function file_exist(path)
    local file = io.open(path, "r")
    if file==nil then
        return false
    end
    io.close(file)
    return true
end

function is_maintenance()
  local path = "/etc/nginx/maintenance.file"
  return file_exist(path)
end
