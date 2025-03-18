local jwt = require "resty.jwt"
local keys = ngx.shared.internal_keys
local jwt_token = ngx.req.get_headers()["x-aab-partnertoken"]
local key = keys:get("public_encryption_key")
local jwt_obj = jwt:verify(key, jwt_token)
if jwt_obj.verified == false and not string.match(jwt_obj.reason, "'exp' claim expired at") then
    return "unknown"
end
return jwt_obj.payload.sub
