local CRYPTED_YAUID_COOKIE_LIST = {
    "F5y8AXyUIS",
    "fUBMvPpesS",
    "Vd1URbKNns",
    "GOPmuGNOSq",
    "IGZtST094R",
    "Iu93OYsfUm",
    "xag1KkQUGc",
    "Sd0j32Tnel",
    "HRtwNvaEs5",
    "H7YMtxGfdy",
    "oS2GvGzEtd",
    "LhfDDwJR8O",
    "ZLPd9IBm9h",
    "fxYguKCegV",
    "HVHCZRNRWD",
    "G4ufM8iJmZ",
    "mt9ovs2y4R",
    "sX3J4lfLop",
    "EdC6GUuhJW",
}

if ngx.var.cookie_yandexuid ~= nil or ngx.var.cookie_crookie ~= nil then
    return ngx.var.cookies
end

local cookie_crookie = nil
local crookie_val = nil

for i = 1, table.getn(CRYPTED_YAUID_COOKIE_LIST) do
    local cookie_name = "cookie_" .. CRYPTED_YAUID_COOKIE_LIST[i]
    if ngx.var[cookie_name] ~= nil then
		cookie_crookie = CRYPTED_YAUID_COOKIE_LIST[i]
        crookie_val = ngx.var["cookie_" .. cookie_crookie]
		break
	end
end

local new_cookies = ngx.var.cookies
if cookie_crookie ~= nil then
	local replace_string = cookie_crookie .. "=" .. crookie_val
	new_cookies = new_cookies:gsub(replace_string, "crookie=" .. crookie_val)
    ngx.var.crookie = crookie_val
end

return new_cookies
