local log = require("log")
local fmt = string.format

-- payload        - table come from juggler sender
--
-- checkName      - string come from juggler config
-- checkDescription   - string come from juggler config
-- variables      - table come from juggler config
-- config         - table come from juggler config (user defined config)
-- conditions       - table come from juggler config OK WARN CRIT ...

function pick(t, tags, ns, ret)
  for k, v in pairs(t) do
    table.insert(ns, k)
    if type(v) == "table" then
      pick(v, tags, ns, ret)
    else
      path = table.concat(ns, ".")
      local srv = {path:match(config.pattern)}
      if #srv > 0 then
        local sName = table.concat(srv, "_")
        log.info(fmt("work with %s", path))
        curSrv = ret[sName] or {names = {}}
        local lastKey = ""..ns[#ns]
        local name = lastKey:match(config.name)
        if name then
          log.debug(fmt("match name %s", name))
          curSrv.names[name] = (curSrv.names[name] or 0) + v
        else
          local divider = ns[#ns]:match(config.divider)
          if divider then
            log.debug(fmt("match divider %s", divider))
            curSrv.divider = (curSrv.divider or 0) + v
          end
        end
        ret[sName] = curSrv
      else
        log.debug(fmt("skip metric %s it not match %s", path, config.pattern))
      end
    end
    table.remove(ns)
  end
end

function run()
  config = config or {}
  config.limits = config.limits or {}
  config.pattern = config.pattern or '^([^%.]+)%.'
  config.name = config.name or "^5xx$"
  config.divider = config.divider or "^total_rps$"
  prefix = ""
  if config.checkNamePrefix then
    prefix = config.checkNamePrefix .. "_"
  end

  log.debug(fmt("Use query: pattern '%s', name = '%s', divider = '%s'",
                config.pattern, config.name, config.divider))
  assert(payload)

  local result = {}

  local level = "OK"
  local description = "OK"

  for _, data in pairs(payload) do
    local events = {}

    if config.type and data.Tags.type ~= config.type then
      log.debug(fmt("skip data for '%s-%s' type %s", data.Tags.metahost, data.Tags.name, data.Tags.type))
    else
      pick(data.Result, data.Tags, {}, events)
      for name, e in pairs(events) do
        for n, v in pairs(e.names) do
          level = "OK"; description = "OK"

          local limit = config.limits[n] or config.limits["default"]
          for _, lvl in pairs {"CRIT", "WARN", "INFO"} do
            if not e.divider then break end

            local curRate = v/e.divider
            local curLimit = limit[lvl] or 1
            if curRate > curLimit then
              level = lvl
              description = fmt("%0.3f > %0.3f", curRate, curLimit)
              break
            end
          end
          log.info(fmt("Trigger state for %s.%s %0.3f/%0.3f == %s -> %s", name, n, v, e.divider, description, level))
          result[#result+1] = {
            tags = data.Tags,
            service = fmt("%s%s_%s", prefix, name, n),
            level = level,
            description = description,
          }
        end
      end
    end
  end
  return result
end
