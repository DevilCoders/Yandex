if cjson == nil then
    cjson = require "cjson"
end

if woothee == nil then
    woothee = require "resty.woothee"
end

local TEMP_NUMERIC_METRICS = {}
local TRANSFER_DELAY = 1.0
local METRIC_GROUPING_TIME = 10
local TTL_TIMESERIES = 2 * METRIC_GROUPING_TIME + 2 * TRANSFER_DELAY

local BROWSER_LIST = {"chrome", "edge", "firefox", "opera", "safari", "unknown", "yandex_browser"}
local AB_BLOCKED_TYPE_LIST = {"element", "network", "iframe", "fake" , "exception", "instant", "unknown"}
local HARDCODED_SERVICE_LIST = {"autoru-test", "gorod-rabot", "yandex-sport", "yandex-afisha", "yandex-images", "yandex-mail", "yandex-morda",
                                "yandex-news", "yandex-player", "yandex-pogoda", "yandex-realty", "yandex-tv", "yandex-video"}


local function list_contains(list, element)
    for _, value in ipairs(list) do
        if value == element then
            return true
        end
    end
    return false
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
        if check_error(key, dict:safe_add(key, 0, TTL_TIMESERIES)) then
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
    while true do
        local temp_table

        temp_table, TEMP_NUMERIC_METRICS = TEMP_NUMERIC_METRICS, {}
        for key, value in pairs(temp_table) do
            increment_shared_dict_value(ngx.shared.numeric_metrics, key, value)
        end

        ngx.sleep(TRANSFER_DELAY + math.random(0, 25) / 100.0 * TRANSFER_DELAY)
    end
end


local function get_host_from_url(url)
    local domain = url:match("^%w+://([^/]+)");
    return domain;
end


ngx.timer.at(TRANSFER_DELAY, transfer_metrics)
math.randomseed(os.time());


function get_service_id_from_naydex(host)
    if host == nil then
        return "unknown"
    end
    local service = split(host, ".")[1]
    if list_contains(HARDCODED_SERVICE_LIST, service) then
        return string.gsub(service, "-", "_")
    end
    return string.gsub(service, "-", ".")
end


function prepare_log_info()
    local log_record = {
        request_id = ngx.var.aab_requestid,
        host = ngx.var.hostname,
        request_time = tonumber(ngx.var.request_time),
        response_code = tonumber(ngx.var.status),
        http_host = ngx.var.host,
        uri_path = ngx.var.request_uri,
        method = ngx.var.request_method,
        http_referer = ngx.var.http_referer,
        yandexuid = ngx.var.cookie_yandexuid,
        crookie = ngx.var.cookie_crookie,
        icookie = ngx.var.cookie_i,
        forcecry = ngx.var.cookie_forcecry,
        x_req_id = ngx.var.http_x_req_id,
        x_request_id = ngx.var.http_x_request_id,
    }
    local service_id = ngx.var.service_id or 'unknown'
    log_record['service_id'] = service_id
    local time_parts = split(ngx.var.msec, '.')
    local time_in_seconds = time_parts[1]
    local solomon_ts = time_in_seconds - time_in_seconds % METRIC_GROUPING_TIME;
    local milliseconds = time_parts[2]
    log_record["@timestamp"] = os.date("!%Y-%m-%dT%T", time_in_seconds)..'.'..milliseconds..'Z';
    log_record["timestamp"] = tonumber(time_in_seconds..milliseconds);
    local browser, device = detect_browser()
    log_record["user_browser_name"] = browser;
    log_record["device"] = device;
    log_record["no_token"] = ngx.var.http_x_aab_partnertoken == nil;
    log_record["userip"] = get_real_ip();
    if ngx.var.http_x_forwarded_for ~= nil then
        log_record["clientip"] = ngx.var.http_x_forwarded_for:match('[^,^ ]+$')  -- last address from x-forwarded-for header
    else
        log_record["clientip"] = ngx.var.remote_addr
    end
    if ngx.var.cookie_cycada ~= nil then
        log_record["has_cycada"] = "1"
    else
        log_record["has_cycada"] = "0"
    end
    if ngx.req.get_method()== "POST" and ngx.req.get_headers()["x-aab-http-check"] ~= nil then
        local bamboozled_json = get_bamboozled_json()
        if bamboozled_json then
            if browser == "yandex_browser" then
                local yscookie =  get_yabro_browser_cookie()
                if yscookie ~= "" then
                    bamboozled_json["data"]["yabro_cookie"] = yscookie
                end
            end
            log_record["bamboozled"] = bamboozled_json
            local action = bamboozled_json["data"]["action"]
            local app = bamboozled_json["ab"]
            local blockId = bamboozled_json["data"]["blockId"]
            local version = bamboozled_json["v"]
            -- blocked type
            local ab_type = nil
            local blocked = bamboozled_json["blocked"]
            if blocked ~= nil then
                local blocked_split = split(blocked)
                ab_type = blocked_split[1]
            end
            if ab_type == nil or not list_contains(AB_BLOCKED_TYPE_LIST, ab_type) then
                ab_type = "unparsed"
            end
            increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#bamboozled&"..service_id.."&"..browser.."&"..action.."&"..app.."&"..device, 1)
            increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#bamboozled_by_ab_type&"..service_id.."&"..action.."&"..device.."&"..ab_type, 1)
            increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#bamboozled_by_block&"..service_id.."&"..browser.."&"..action.."&"..app.."&"..blockId, 1)
            increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#bamboozled_by_pcode&"..service_id.."&"..action.."&"..version, 1)
        end
    end
    increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#request&"..service_id.."&"..ngx.status.."&"..browser, 1)
    if ngx.var.is_accel_redirect == "1" then
        local host_from_accel_redirect = get_host_from_url(ngx.var.decrypted_url);
        log_record['ar_host'] = host_from_accel_redirect;
        increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#accelredirect&"..service_id.."&"..ngx.status.."&"..host_from_accel_redirect, 1)
    end
    ngx.var.log_record_by_lua = cjson.encode(log_record)
end

function parse_headers()
    local headers_str = "";
    local h = ngx.req.get_headers();
    for k, v in pairs(h) do
        if k ~= "cookie" and k ~= "authorization" and k ~= "x-ya-service-ticket" and k ~= "x-ya-user-ticket" then
            headers_str = headers_str .. k .. ": " .. v .. "\r\n"
        end
    end
    local yandexuid = ngx.var.cookie_yandexuid;
    local crookie = ngx.var.cookie_crookie;
    local icookie = ngx.var.cookie_i;
    local forcecry = ngx.var.cookie_forcecry;
    local cookie = "";
    if yandexuid ~= nil then
        cookie = cookie .. "yandexuid=" .. yandexuid .. ";"
    end
    if crookie ~= nil then
        cookie = cookie .. "crookie=" .. crookie .. ";"
    end
    if icookie ~= nil then
        cookie = cookie .. "i=" .. icookie .. ";"
    end
    if forcecry ~= nil then
        cookie = cookie .. "forcecry=" .. forcecry .. ";"
    end
    if cookie ~= "" then
        headers_str = headers_str .. "cookie: " .. cookie .. "\r\n";
    end
    return headers_str;
end

function prepare_request_log_info()
    local headers = parse_headers();
    local full_request = ngx.var.request .. "\r\n" .. headers .. "\r\n";
    if ngx.req.get_method() == "POST" then
        local request_body = ngx.req.get_body_data();
        if request_body ~= nil then
            full_request = full_request .. request_body;
        end
    end
    local request_log_record = {
        request_id = ngx.var.aab_requestid,
        request_time = tonumber(ngx.var.request_time),
        response_code = tonumber(ngx.var.status),
        host = ngx.var.hostname,
        request = full_request,
        is_accel_redirect = ngx.var.is_accel_redirect,
        is_bamboozled = ngx.var.is_bamboozled,
    }
    local time_parts = split(ngx.var.msec, '.')
    request_log_record["timestamp"] = tonumber(time_parts[1]..time_parts[2]);
    if ngx.var.is_accel_redirect == "1" then
        request_log_record["response_headers"] = ngx.resp.get_headers();
        request_log_record["response_body"] = ngx.var.response_body
    end
    ngx.var.request_log_record_by_lua = cjson.encode(request_log_record)
end


function check_additional_loggable(probability_seed)
    probability_seed = probability_seed or 0
    if (ngx.var.aab_requestid ~= nil and probability_seed > 0) then
        return math.random(probability_seed) == 1
    else
        return false
    end
end


local BAMBOOZLED_EVENTS = {'old_block_confirmation', 'block_confirmation'}


local function validate_bamboozled(decoded_json)
    if list_contains(BAMBOOZLED_EVENTS, decoded_json["event"]) then
        return true
    else
        return false
    end
end


local ADBLOCK_APPS = {'ADBLOCK', 'ADBLOCKPLUS', 'ADGUARD', 'GHOSTERY', 'NOT_BLOCKED', 'UBLOCK', 'UK', 'UNKNOWN', 'FF_PRIVATE', 'KIS', 'DNS', 'OPERA_BROWSER', 'BRAVE', 'UCBROWSER'}
local BAMBOOZLED_ACTIONS = {'confirm_block', 'try_to_render', 'try_to_load'}


function get_bamboozled_json()
    local success, json = pcall(cjson.decode, ngx.req.get_body_data())
    if not success or not validate_bamboozled(json) then
        return false
    end
    if not list_contains(ADBLOCK_APPS, json["ab"]) then
        json["ab"] = "UNPARSED"
    end
    if json["data"] == nil then
        json["data"] = {}
    end
    if not list_contains(BAMBOOZLED_ACTIONS, json["data"]["action"]) then
        json["data"]["action"] = "unparsed"
    end
    if json["data"]["blockId"] == nil or string.match(json["data"]["blockId"], '^%a?%-?%a?%-?%d+%-?%d-$') == nil then
        json["data"]["blockId"] = "unparsed"
    end
    if json["v"] == nil or string.match(json["v"], '^%d+$') == nil then
        json["v"] = "unparsed"
    end
    json["data"]["check_detect_cookie"] = check_detect_cookie()
    return json
end


function get_yabro_browser_cookie()
    local yscookie = ngx.var.cookie_ys or ""

    return yscookie:match("#ead.([^#]+)") or yscookie:match("^ead.([^#]+)") or ""
end


function check_detect_cookie()
    local result = 0;
    if ngx.var.cookie_computer ~= nil then
        result = result + 1
    end
    if ngx.var.cookie_instruction ~= nil then
        result = result + 2
    end
    return result
end


function detect_browser()
    if ngx.var.http_user_agent then
        local user_agent_parsed = woothee.parse(ngx.var.http_user_agent)
        local device = user_agent_parsed.category
        if device == "crawler" then
            return device, device
        end
        local user_agent_name = string.gsub(user_agent_parsed.name:lower(), " ", "_")
        if not list_contains(BROWSER_LIST, user_agent_name) then
            return "other", device
        end
        return user_agent_name, device
    end
    return "unknown", "unknown"
end


function split(inputstr, sep)
    if sep == nil then
        sep = "%s"
    end
    local t = {}
    local i = 1
    for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
        t[i] = str
        i = i + 1
    end
    return t
end


local function mySort(a,b)
    return a['ts'] < b['ts']
end


function get_solomon_metrics(sensor_type)
    local ts_now = ngx.time()
    ts_now = ts_now - 2 * TRANSFER_DELAY
    local sensor_ts = ts_now - ts_now % METRIC_GROUPING_TIME - METRIC_GROUPING_TIME
    local numerics = ngx.shared.numeric_metrics:get_keys(0)
    local prepare_ts = {}
    local output = {}
    for idx = 1, #numerics do
        if (sensor_type == "request" and (string.match(numerics[idx], "%d+#request.+") or string.match(numerics[idx], "%d+#accelredirect.+"))) or
           (sensor_type == "bamboozled" and string.match(numerics[idx], "%d+#bamboozled.+")) or
           (sensor_type == "detect" and string.match(numerics[idx], "%d+#detect.+")) then
            local split_time = split(numerics[idx], "#")
            local metric_time = tonumber(split_time[1])
            if metric_time == sensor_ts then
                if prepare_ts[split_time[2]] == nil then
                    prepare_ts[split_time[2]] = {}
                end
                table.insert(prepare_ts[split_time[2]], { ts = metric_time, value = ngx.shared.numeric_metrics:get(numerics[idx]) / METRIC_GROUPING_TIME })
            end
        end
    end
    for key, value in pairs(prepare_ts) do
        local labels = split(key, "&")
        local sensor = labels[1]
        table.sort(value, mySort)
        local service_id = labels[2]
        if sensor_type == "bamboozled" then
            if sensor == "bamboozled" then
                output[#output + 1] = {labels={sensor=sensor, service_id=service_id, browser=labels[3], action=labels[4], app=labels[5], device=labels[6], host='const'}, timeseries=value, kind="IGAUGE"}
            elseif sensor == "bamboozled_by_ab_type" then
                output[#output + 1] = {labels={sensor=sensor, service_id=service_id, action=labels[3], device=labels[4], ab_type=labels[5], host='const'}, timeseries=value, kind="IGAUGE"}
            elseif sensor == "bamboozled_by_block" then
                output[#output + 1] = {labels={sensor=sensor, service_id=service_id, browser=labels[3], action=labels[4], app=labels[5], blockId=labels[6], host='const'}, timeseries=value, kind="IGAUGE"}
            elseif sensor == "bamboozled_by_pcode" then
                output[#output + 1] = {labels={sensor=sensor, service_id=service_id, action=labels[3], version=labels[4], host='const'}, timeseries=value, kind="IGAUGE" }
            end
        elseif sensor_type == "request" then
            if sensor == "request" then
                output[#output + 1] = {labels={sensor=sensor, service_id=service_id, status=labels[3], browser=labels[4]}, timeseries=value, kind="IGAUGE"}
            elseif sensor == "accelredirect" then
                output[#output + 1] = {labels={sensor=sensor, service_id=service_id, status=labels[3], http_host=labels[4]}, timeseries=value, kind="IGAUGE"}
            end
        elseif sensor_type == "detect" then
            output[#output + 1] = {labels={blocker=labels[2], element=labels[3], pid=labels[4], browser=labels[5], device=labels[6], version=labels[7], host='const'}, timeseries=value, kind="IGAUGE"}
        end
    end
    if next(output) == nil then
        return '{"sensors": []}' -- because cjson encodes empty table as object
    end
    return cjson.encode({sensors=output})
end


function get_real_ip()
    if ngx.var.http_x_real_ip ~= nil then
        return ngx.var.http_x_real_ip;
    elseif ngx.var.http_x_forwarded_for ~= nil then
        -- Get real ip as a first address from x-forwarded-for header
        return ngx.var.http_x_forwarded_for:match('^[^,]+');
    else
        return ngx.var.remote_addr;
    end
end


local function validate(json, validator)
    for field, check in pairs(validator) do
        local value = json[field]
        if value == nil then
            return false
        end
        if type(check) == "string" then
            if type(value) ~= check then
                return false
            end
        elseif type(check) == "table" then
            if not validate(json[field], validator[field]) then
                return false
            end
        end
    end
    return true
end


local function validate_detect(json)
    local validator = {
        -- service = "string",
        -- tags = "table",
        labels = {
            blocker = "string",
            element = "string",
            pid = "string",
            browser = "string",
            device = "string",
            version = "string",
        },
        -- timestamp = "number",
        -- eventType = "string",
        -- eventName = "string",
        -- data = "table",
        -- sid = "string",
        -- version = "string",
        -- location = "string",
        -- topLocation = "string",
        -- pn = "string",
        -- ancestorOrigins = "table",
        -- gn = "number",
        -- referrer = "string",
        -- topReferrer = "string",
        -- currentScriptSrc = "string",
        -- mn = "number",
        -- Sn = "number",
        -- value = "number",
        -- Tn = "number",
        -- kn = "number",
    }
    return validate(json, validator)
end


local function get_detect_json()
    local success, json = pcall(cjson.decode, ngx.req.get_body_data())
    if not success or not validate_detect(json) then
        return nil
    end
    return json
end


function log_detect()
    if ngx.req.get_method() ~= "POST" then
        return
    end

    local time_parts = split(ngx.var.msec, '.')
    local time_in_seconds = time_parts[1]
    local solomon_ts = time_in_seconds - time_in_seconds % METRIC_GROUPING_TIME
    local detect_json = get_detect_json()
    if detect_json ~= nil then
        local labels = detect_json["labels"]

        local blocker = labels["blocker"]
        local element = labels["element"]
        local pid = labels["pid"]
        local browser = labels["browser"]
        local device = labels["device"]
        local version = labels["version"]
        increment_dict_value(TEMP_NUMERIC_METRICS, solomon_ts.."#detect&"..blocker.."&"..element.."&"..pid.."&"..browser.."&"..device.."&"..version, 1)
    end
end
