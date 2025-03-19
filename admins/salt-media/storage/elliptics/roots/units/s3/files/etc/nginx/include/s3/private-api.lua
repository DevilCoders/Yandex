local JSON = require "cjson.safe"
local S3Files = require "s3.files"
local S3Log = require "s3.log"
local HTTP = require "resty.http"

local PrivateAPI = {}
local privateApiMT = { __index = PrivateAPI }

local CLIENT_CONFIG = "/etc/nginx/include/s3/private-api.json"
local DEFAULT_ENDPOINT_ADDR = "https://s3-idm.mds.yandex.net"
local DEFAULT_ENDPOINT_HOST = "s3-idm.mds.yandex.net"

--- Encode LUA table into HTTP query args string.
--- @param args table - arguments to convert
--- @return string
local function encode_args(args)
    if not args or type(args) ~= "table" then
        return ""
    end

    return ngx.encode_args(args)
end

--- Create private API client
--- @return table - Private API client
function PrivateAPI.new(_)
    local config = {}
    local err

    if S3Files.exists(CLIENT_CONFIG) then
        config, err = S3Files.readall_json(CLIENT_CONFIG)
        if err then
            S3Log.Error("can't load S3 Private client configuration: " .. tostring(err))
            config = {}
        end
    end

    local client = {
        client = HTTP:new(),
        endpoint = config["endpoint"] or DEFAULT_ENDPOINT_ADDR,
        host_header = config["host_header"] or DEFAULT_ENDPOINT_HOST,
        oauth_token = config["oauth_token"] or nil,
    }

    return setmetatable(client, privateApiMT)
end

--- Generate default headers for each S3 API request.
--- @return table - standard headers to be sent in each request by current client
function PrivateAPI._headers(self)
    local headers = {}
    if self.host_header then
        headers["Host"] = self.host_header
    end

    if self.oauth_token then
        headers["Authorization"] = "OAuth " .. self.oauth_token
    end

    return headers
end


--- Send 'GET' HTTP request to the given API method
--- @param self table - S3 Private API client obtained from PrivateAPI:new()
--- @return table, string - <result>, <error>
function PrivateAPI.get(self, method)
    local headers = self:_headers()

    local res, err = self.client:request_uri(self.endpoint .. method, {
        method = "GET",
        headers = headers,
    })

    if err then
        return res, err
    end

    if res.status < 200 or res.status >= 300 then
        return res, res.body
    end

    local data
    data, err = JSON.decode(res.body)

    if err then
        return nil, "failed to parse S3 Private API '" .. method .. "' response as JSON: " .. tostring(err)
    end

    return data, nil
end

--- Get list of opened buckets
--- @param self table - S3 Private API client obtained from PrivateAPI:new()
--- @param filters table - filters to apply, e.g. open_private = 'true' will make API to return only private buckets
function PrivateAPI.buckets_opened(self, filters)
    local args = encode_args(filters)
    local method = "/buckets/opened"
    if args ~= "" then
        method = method .. "?" .. args
    end

    local result, err = self:get(method)
    if err then
        return result, err
    end

    return result["buckets"], nil
end

return PrivateAPI
