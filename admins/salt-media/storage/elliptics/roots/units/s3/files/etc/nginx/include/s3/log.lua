local Logger = {}

local function logWithPrefix(lvl, msg)
    ngx.log(lvl, "[S3LuaLib]: " .. msg)
end

function Logger.Stderr(msg)
    logWithPrefix(ngx.STDERR, msg)
end

function Logger.Emergency(msg)
    logWithPrefix(ngx.EMERG, msg)
end

function Logger.Alert(msg)
    logWithPrefix(ngx.ALERT, msg)
end

function Logger.Crit(msg)
    logWithPrefix(ngx.CRIT, msg)
end

function Logger.Error(msg)
    logWithPrefix(ngx.ERR, msg)
end

function Logger.Warning(msg)
    logWithPrefix(ngx.WARN, msg)
end

function Logger.Notice(msg)
    logWithPrefix(ngx.NOTICE, msg)
end

function Logger.Info(msg)
    logWithPrefix(ngx.INFO, msg)
end

function Logger.Debug(msg)
    logWithPrefix(ngx.DEBUG, msg)
end

return Logger
