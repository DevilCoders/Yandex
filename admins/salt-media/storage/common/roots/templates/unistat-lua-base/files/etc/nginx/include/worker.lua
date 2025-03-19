if cjson == nil then
    cjson = require "cjson"
end


local HISTOGRAM_LOG_BASE = 1.5
local HISTOGRAM_MIN_LOG = -50
local HISTOGRAM_MAX_LOG = 50
local HISTOGRAM_BORDERS = {0}
for idx = -50, 50 do
    HISTOGRAM_BORDERS[idx+50+2] = 1.5 ^ idx
end

local TEMP_HISTOGRAM_VALUES = {}
local TEMP_HISTOGRAM_NAMES = {}
local TEMP_NUMERIC_METRICS = {}
local TRANSFER_DELAY = 1.0


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
    while not ngx.worker.exiting() do
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
        ngx.sleep(TRANSFER_DELAY + math.random(0, 25) / 100.0 * TRANSFER_DELAY)
    end
end

ngx.timer.at(TRANSFER_DELAY, transfer_metrics)


function increment_metric(name, value)
    if value == nil then
        ngx.log(ngx.ERR, "Trying to set nil value for metric " .. name)
    else
        increment_dict_value(TEMP_NUMERIC_METRICS, name, value)
    end
end


function add_to_histogram(name, value)
    local offset = 0
    if value == nil then
        ngx.log(ngx.ERR, "Trying to add nil value to histogram for metric " .. name)
        return
    else
        offset = math.floor(math.max(HISTOGRAM_MIN_LOG - 1, math.min(HISTOGRAM_MAX_LOG, math.log(value) / math.log(HISTOGRAM_LOG_BASE))) - HISTOGRAM_MIN_LOG + 1) + 1
    end

    increment_dict_value(TEMP_HISTOGRAM_VALUES, name .. offset, 1)
    TEMP_HISTOGRAM_NAMES[name] = 1
end


local function yasmify_histogram(name)
    local hvalues = ngx.shared.histogram_values
    local output = {}
    for idx = 1, #HISTOGRAM_BORDERS do
        local value = hvalues:get(name .. idx)
        if value ~= nil then
            output[#output+1] = {HISTOGRAM_BORDERS[idx], value}
        end
    end
    return output
end


function get_yasm_metrics()
    local numerics = ngx.shared.numeric_metrics:get_keys(0)
    local output = {}
    for idx = 1, #numerics do
        local value = ngx.shared.numeric_metrics:get(numerics[idx])
        output[#output + 1] = { numerics[idx], value }
    end

    local histograms = ngx.shared.histogram_names:get_keys(0)
    for idx = 1, #histograms do
        local value = yasmify_histogram(histograms[idx])
        output[#output + 1] = { histograms[idx], value }
    end

    return output
end
