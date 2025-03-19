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

local TEMP_SOLOMON_HISTOGRAM_VALUES = {}
local TEMP_SOLOMON_HISTOGRAM_NAMES = {}
local TEMP_SOLOMON_NUMERIC_METRICS = {}

local TEMP_SOLOMON_CLIENT_NUMERIC_METRICS = {}

local function dict_keys(dict)
    local keys = {}
    for k in pairs(dict) do
        table.insert(keys, k)
    end
    table.sort(keys)
    return keys
end

local function dict_to_string(dict)
    keys = dict_keys(dict)
    local result = ""
    for idx = 1, #keys do
        result = result .. keys[idx] .. "=" .. dict[keys[idx]] .. ";"
    end
    return result
end

local function string_to_dict(str)
    local dict = {}
    for tag in string.gmatch(str, "[^;]+") do
        for key, value in string.gmatch(tag, "([^=]+)=([^=]+)") do
            dict[key] = value
        end
    end
    return dict
end

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


        temp_table, TEMP_SOLOMON_NUMERIC_METRICS = TEMP_SOLOMON_NUMERIC_METRICS, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.solomon_numeric_metrics, key, value)
        end
        temp_table, TEMP_SOLOMON_HISTOGRAM_VALUES = TEMP_SOLOMON_HISTOGRAM_VALUES, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.solomon_histogram_values, key, value)
        end

        temp_table, TEMP_SOLOMON_HISTOGRAM_NAMES = TEMP_SOLOMON_HISTOGRAM_NAMES, {}
        for key in pairs(temp_table) do
            check_error(key, ngx.shared.solomon_histogram_names:safe_set(key, 1))
        end
        ngx.sleep(TRANSFER_DELAY + math.random(0, 25) / 100.0 * TRANSFER_DELAY)


        temp_table, TEMP_SOLOMON_CLIENT_NUMERIC_METRICS = TEMP_SOLOMON_CLIENT_NUMERIC_METRICS, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.solomon_client_numeric_metrics, key, value)
        end

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

function increment_solomon_metric(tags, value)
    tags_string = dict_to_string(tags)
    if value == nil then
        ngx.log(ngx.ERR, "Trying to set nil value for metric " .. tags_string)
    else
        increment_dict_value(TEMP_SOLOMON_NUMERIC_METRICS, tags_string, value)
    end
end

function increment_solomon_client_metric(tags, value)
    tags_string = dict_to_string(tags)
    if value == nil then
        ngx.log(ngx.ERR, "Trying to set nil value for metric " .. tags_string)
    else
        increment_dict_value(TEMP_SOLOMON_CLIENT_NUMERIC_METRICS, tags_string, value)
    end
end

function add_to_histogram(name, value)
    local offset = 0
    if value == nil then
        ngx.log(ngx.ERR, "Trying to add nil value to histogram for metric " .. name)
        return
    elseif value ~= 0 then
        offset = math.floor(math.max(HISTOGRAM_MIN_LOG - 1, math.min(HISTOGRAM_MAX_LOG, math.log(value) / math.log(HISTOGRAM_LOG_BASE))) - HISTOGRAM_MIN_LOG + 1) + 1
    end

    increment_dict_value(TEMP_HISTOGRAM_VALUES, name .. offset, 1)
    TEMP_HISTOGRAM_NAMES[name] = 1
end

function add_to_solomon_histogram(tags, value)
    local tags_string = dict_to_string(tags)
    local offset = 0
    if value == nil then
        ngx.log(ngx.ERR, "Trying to add nil value to histogram for metric " .. tags_string)
        return
    elseif value ~= 0 then
        offset = math.floor(math.max(HISTOGRAM_MIN_LOG - 1, math.min(HISTOGRAM_MAX_LOG, math.log(value) / math.log(HISTOGRAM_LOG_BASE))) - HISTOGRAM_MIN_LOG + 1) + 1
    end

    increment_dict_value(TEMP_SOLOMON_HISTOGRAM_VALUES, tags_string .. offset, 1)
    TEMP_SOLOMON_HISTOGRAM_NAMES[tags_string] = 1
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

local function solomonify_histogram(name)
    local hvalues = ngx.shared.solomon_histogram_values
    local output = {}
    for idx = 1, #HISTOGRAM_BORDERS do
        local value = hvalues:get(name .. idx)
        if value ~= nil then
            output[name .. "timings_bucket=" .. math.floor(HISTOGRAM_BORDERS[idx]*1000 + 0.5) .. ";"] = value
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

function get_solomon_metrics()
    local numeric = ngx.shared.solomon_numeric_metrics:get_keys(0)
    local output_metrics = {}
    for idx = 1, #numeric do
        local value = ngx.shared.solomon_numeric_metrics:get(numeric[idx])
        output_metrics[#output_metrics + 1] = { string_to_dict(numeric[idx]), value }
    end

    local histograms = ngx.shared.solomon_histogram_names:get_keys(0)
    for idx = 1, #histograms do
        local hgram = solomonify_histogram(histograms[idx])
        for k, v in pairs(hgram) do
            output_metrics[#output_metrics + 1] = {string_to_dict(k), v}
        end
    end

    return output_metrics
end


function get_solomon_client_metrics()
    local numeric = ngx.shared.solomon_client_numeric_metrics:get_keys(0)
    local output_metrics = {}
    for idx = 1, #numeric do
        local value = ngx.shared.solomon_client_numeric_metrics:get(numeric[idx])
        output_metrics[#output_metrics + 1] = { string_to_dict(numeric[idx]), value }
    end

    return output_metrics
end
