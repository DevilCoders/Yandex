local check_strings = require 'luacheck'.check_strings
local format = require 'luacheck.format'.format

return function(code)
    if _G.luacheck_f_name == nil then
        _G.luacheck_f_name = 'TODO.lua'
    end

    if _G.luacheck_config == nil then
        _G.luacheck_config = {}
    end

    local report = check_strings({code}, _G.luacheck_config)
    local issues = report.fatals + report.errors + report.warnings
    local ok = (issues == 0)

    local message = format(report, {_G.luacheck_f_name}, {color=false})
    return ok, message
end
