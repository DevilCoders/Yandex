local re = require("re")
local log = require("log")
local os = require("os")

-- global vars
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
  for name, def in pairs(variables) do
    func = loadstring("return ".. def)
    setfenv(func, sandbox)
    ok, res = pcall(func)
    if ok then
      -- log.debug("for %s result is: %s: %s",name, ok,res)
      envVars[name] = res
    else
      log.error("Failed to eval %s: %s", def, res)
    end
  end
end


function extractMetric(q, res)
  local metric = "?"
  local iv = res
  local key
  for key in q:gmatch([==[%[['"%s]*([^"'%[%s%]]+)['"%s]*%]]==]) do
    -- log.debug("Query %s on %s with key %s", q, iv, key)
    if key:match("^%-?%d$") then
      key = tonumber(key)
      if key < 0
      then key = #iv -- lua do not support keys -1 like python
      else key = key + 1 -- lua start index from 1, but python from 0
      end
    end
    iv = iv[key]
    if not iv then break end -- payload not contains sub table for given key
  end
  metric = iv or 0
  return metric
end

function addResult(ev, lvl, tags)
  local q, v = ev:match("(.+)([<>!]=?.*)")
  local f, e = loadstring("return "..tostring(q))
  if type(f) == "function" then
    setfenv(f, {})
  end
  local desc = ev
  if not e then
    local _, result = pcall(f)
    desc = string.format("%0.3f%s", result, v)
  end
  desc = desc:gsub("%s+", "")
  log.debug("Eval Query is '%s' q='%s' v='%s' evaluated r='%s'", ev, q, v, desc)
  if checkDescription then
    desc = desc .. " " .. checkDescription
  end

  return {
    tags = tags,
    description = desc,
    level = lvl,
    service = checkName,
  }
end

function evalCheck(cases, lvl, d)
  local pattern = "[$]{"..d.Tags.aggregate.."}"..[=[(?:[[][^]]+[]])+]=]

  for _, c in pairs(cases) do
    local eval = c
    local matched = false
    for query in re.gmatch(c, pattern) do
      matched = true
      local metric = extractMetric(query, d.Result)
      eval = replace(eval, query, tostring(metric))
    end
    if matched then
      local test, err = loadstring("return "..eval)
      if type(test) == "function" then
        setfenv(test, envVars) -- envVars defined at top
      end

      if err then
        log.error("'%q' in check '%q' resolved to -> '%q'", err, c, eval)
      else
        local ok, fire = pcall(test)
        if not ok then
          log.error("Error in query "..eval)
        else
          log.info("%s %s trigger test `%s` -> `%s` is %s", d.Tags.name, lvl, c, eval, fire)
          if fire then
            -- Checks are coupled with OR logic.
            -- return when one of expressions is evaluated as True
            return addResult(eval, lvl, d.Tags)
          end
        end
      end
    else
      log.debug("case '%s' not match, pattern='%s' for '%s'",c, pattern, lvl)
    end
  end
end

function checkHistory(history, level)
  for _, lvl in pairs(history) do
    if lvl ~= level then
      log.info("Reset level to '%s' by events_history [%s]", lvl, table.concat(history, ", "))
      return lvl
    end
  end
  return level
end

function pushEvent(data, event, config)
  if config.history then
    -- checkName is global see top comment
    key = data.Tags.metahost..":"..data.Tags.name..":"..checkName
    log.debug("Push event %s: %s", key, event)
    return events_history(key, event, config.history)
  end
  return nil
end

function run()
  local result = {}
  config = config or {}

  evalVariables()   -- now eval variables

  for _, data in pairs(payload) do
    if config.type and data.Tags.type ~= config.type then
      log.info("skip data for %s-%s type %s", data.Tags.metahost, data.Tags.name, data.Tags.type)
    else
      for idx, level in pairs {"CRIT", "WARN", "INFO", "OK", "DEFAULT"} do
        local check = conditions[level]
        if check then
          local res = evalCheck(check, level, data)
          if res then
            -- Events History push and check
            prevLevels = pushEvent(data, res.level, config)
            if prevLevels then
              if res.level == "CRIT" then
                res.level = checkHistory(prevLevels, res.level)
              end
            end

            result[#result+1] = res
            break -- pairs {"CRIT... higher level event set on fire, no need continue checks
          end
        end
        if idx == 5 then -- idx 5 == "DEFAULT"
          log.info("%s: %s.%s set default status 'OK'",
            data.Tags.aggregate, data.Tags.name, checkName
          )
          -- Events History push
          pushEvent(data, "OK", config)

          result[#result+1] = {
            tags = data.Tags,
            description = "OK",
            level = "OK",
            service = checkName,
          }
        end
      end
    end
  end
  return result
end
