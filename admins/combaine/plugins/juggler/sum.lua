local log = require("log")
local os = require("os")
local fmt = string.format

-- global vars
levelsOpts = {CRIT=">", WARN=">", INFO=">", OK="<", default="="}
--
-- payload        - table come from juggler sender
--
-- checkName      - string come from juggler config
-- checkDescription   - string come from juggler config
-- variables      - table come from juggler config
-- config         - table come from juggler config (user defined config)
-- conditions       - table come from juggler config OK WARN CRIT ...

local envVars = {}      -- evaled vars from variables
local sandbox = {       -- sandbox for function evalVariables
  iftimeofday = function (bot, top, in_val, out_val)
    function inTimeOfDay()
      hour = tonumber(os.date("%H"))
      if bot < top then
        return bot <= hour and hour <= top
      else
        return bot <= hour or hour < top
      end
    end

    if inTimeOfDay(bot, top) then
      return in_val
    else
      return out_val
    end
  end,
}

function evalVariables()
  for name, def in pairs(variables or {}) do
    local func = loadstring("return ".. def)
    setfenv(func, sandbox)
    local ok, res = pcall(func)
    if ok then
      -- log.debug(fmt("for %s result is: %s: %s",name, ok,res))
      envVars[name] = res
    else
      log.error(fmt("Failed to eval %s: %s", def, res))
    end
  end
end

function sum(r, q, ns, qSum)
  local ns = ns or {}
  local ret = qSum or 0
  for k, v in pairs(r) do
    table.insert(ns, k)
    if type(v) == "table" then
      ret = ret + sum(v, q, ns, qSum)
    else
      path = table.concat(ns, ".")
      if path:match(q) then
        ret = ret + tonumber(v)
      end
    end
    table.remove(ns)
  end
  return ret
end

function run()
  config = config or {}
  config.limits = config.limits or {}
  config.query = config.query or '.*'
  log.debug(fmt("Use query: %s", config.query))
  assert(payload)

  evalVariables()   -- now eval variables

  local result = {}
  for _, data in pairs(payload) do
    local setDefault = config.limits.default
    if config.type and data.Tags.type ~= config.type then
      log.debug(fmt("skip data for %s-%s type %s", data.Tags.metahost, data.Tags.name, data.Tags.type))
    else
      local res = sum(data.Result, config.query)
      for _, level in pairs {"CRIT", "WARN", "INFO", "OK"} do
        local limit = config.limits[level]
        if limit then
          local fire
          if levelsOpts[level] == ">" then
            fire = res > limit
          else
            fire = res < limit
          end
          if fire then
            setDefault = false
            result[#result+1] = {
              tags = data.Tags,
              description = fmt("%0.3f %s %0.3f", res, levelsOpts[level], limit),
              level = level,
              service = checkName,
            }
            -- higher level event set on fire, no need continue checks
            break
          end
        end
      end
      if setDefault then
        log.debug(fmt("%s set default for %s", data.Tags.aggregate, data.Tags.metahost))
        result[#result+1] = {
          tags = data.Tags,
          description = "OK",
          level = config.limits.default,
          service = checkName,
        }
      end
    end
  end
  return result
end
