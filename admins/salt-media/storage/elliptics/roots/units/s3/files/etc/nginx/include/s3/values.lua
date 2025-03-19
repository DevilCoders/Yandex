local S3Values = {}

local function ternary(cond, T, F)
    if cond then
        return T
    else
        return F
    end
end

-- Variables calculated by S3Lib itself.
-- They should be declared inside nginx config by 'set' directive.
-- Use 'include values.conf;' for all required variables initialization
-- @see S3Lib.init
---@return boolean
function S3Values.isInitialized() return ngx.var.s3_lib_initialized == "1" end
function S3Values.setInitialized() ngx.var.s3_lib_initialized = "1" end

---@return string
function S3Values.getBucketName() return ngx.var.s3_bucket_name end
---@param v string
function S3Values.setBucketName(v) ngx.var.s3_bucket_name = v end

---@return boolean
function S3Values.isPublicRequest() return ngx.var.s3_public_request == "1" end
---@param v boolean
function S3Values.setPublicRequest(v) ngx.var.s3_public_request = ternary(v, "1", "0") end

--
-- Request info getters
-- This group of getters provide data to request info initialized by NGINX core
-- or received from client in request parameters
--
---@return string
function S3Values.getHost()             return ngx.var.host end
---@return number
function S3Values.getServerPort()       return tonumber(ngx.var.server_port) end
---@return string
function S3Values.getURI()              return ngx.var.uri end
---@return string
function S3Values.getRequestURI()       return ngx.var.request_uri end
---@return number
function S3Values.getRequestLength()    return tonumber(ngx.var.request_length) end

--- @return string
function S3Values.getHeaderAuthorization() return ngx.var.http_authorization or "" end

---@return boolean
function S3Values.isWriteRequest()
    return ngx.var.request_method == "PUT"
        or ngx.var.request_method == "POST"
        or ngx.var.request_method == "DELETE"
end
---@return boolean
function S3Values.isReadRequest()
    return ngx.var.request_method == "GET"
        or ngx.var.request_method == "HEAD"
        or ngx.var.request_method == "OPTIONS"
end

---@return boolean
function S3Values.isPrioritizedRequest()
    return ngx.var.arg_prioritypass ~= nil or ngx.var.http_x_yandex_prioritypass ~= nil
end

--- YARL quotas environment. Affects quota names used by NGINX when limiting clients by
--- request rate or bandwidth.
---@return string - single letter with sign of current environment: 'p', 't' and so on
function S3Values.getYarlEnv()
    if ngx.var.s3_yarl_env == nil or ngx.var.s3_yarl_env == "" then
        return "d"
    end

    return tostring(ngx.var.s3_yarl_env)
end

return S3Values
