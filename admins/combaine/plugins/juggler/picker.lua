local log = require("log")
local os = require("os")

--
-- payload        - приходит из juggler сендера
--  это list в форме
--  [
--   {
--   Tags: {теги}
--   Data:{
--       длинный.ключ.с.названием.метрики: 10
--       длинный.ключ.для.таймингов: [1, 2, 3]
--     }
--   }
--   ...
--  ]
-- так как в lua все сложные структуры данных - это таблицы,
-- тайминги представляются как ключ с диктом (таблицей) в значении.
--
-- остальное - это глобальные переменные, которые приходят из конфиги комбайна
-- из описания про секцию juggler сендера
-- checkName          - string come from juggler config
-- checkDescription   - string come from juggler config
-- checkTTL           - number come from juggler config
-- variables          - table come from juggler config
-- config             - table come from juggler config (user defined config)

local triggerDescription = "%s: %0.3f > %0.3f"
local triggerDescriptionPrc = "%s: %0.3f%% > %0.3f%%"

local envVars = {}      -- evaled vars from variables
-- сюда можно добавить других функций хелперов, на основе которых
-- будут изменяться значения переменных заданных в конфиге juggler сендера
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

-- helper
function evalVariables()
  for name, def in pairs(variables or {}) do
    local func = loadstring("return ".. def)
    setfenv(func, sandbox)
    local ok, res = pcall(func)
    if ok then
      log.info("resolve result is %s: %s=%s", ok, name, res)
      envVars[name] = res
    else
      log.error("Failed to eval %s: %s", def, res)
    end
  end
end

function getThresholdFromSenderConfigs(nameOfService, description)
    -- дальнейший кусок относится к логике обработки конфигурации juggler
    -- сендера описаной в конфиге комбайна.
    local default = config.limits["default"] -- config - это глобальная переменная
    -- get threshold for current pathString
    local threshold = config.limits[nameOfService]
    if not threshold then  -- in lua only nil and false are false values
      -- if got nil or false (false almost imposible here), try use default
      threshold = default
    end
    if type(threshold) == "table" then
      for_desc = description
      oldStylelimit = threshold[for_desc]
      if not oldStylelimit and threshold["default"] then
        for_desc = "default"
        oldStylelimit = threshold[for_desc]
      end
      if oldStylelimit then
        log.debug(
          "threshold description for '%s.%s' looks like oldStylelimit: {name: {description: <limit>}}",
          nameOfService, for_desc
        )
        threshold = oldStylelimit
      end
      if type(threshold) == "table" then
        newDescription = threshold.explain or threshold[2]
        if newDescription then
          description = description .. ": " .. newDescription
        end
        threshold = threshold.limit or threshold[1] -- override threshold
        if not threshold and default then
          -- last resort bilat', got it actual default value
          -- work well for old style limits and new style limits
          -- old style (N - is number)
          -- limits:
          --   "default": N
          --   <nameOfService>:
          --      <description>: N
          --
          -- for new style common case is query
          -- where nameOfService and description are same submatch like '((pattern))'
          -- new style:
          --   "default": N
          --   <nameOfService>: [<N (threshold)>, <description>]
          --   or
          --   <nameOfService>: {limit: <N (threshold)>, explain: <description>}
          threshold = default
        end
      end
    end -- законфили с конфигой

    -- теперь проверяем, если лимит - это переменная (например из iftimeofday)
    if type(threshold) == "string" then
      -- резолвим значение этой переменной
      newLimit = envVars[threshold] -- envVars - глобальная переменная
      if type(newLimit) ~= "number" then -- если newLimit не число
        log.error("failed to resolve variable, got %s=%s", threshold, newLimit)
        newLimit = nil
      end
      threshold = newLimit
    end

    return threshold, description
end

function computePercentage(currentTable, key, value)
  local template = triggerDescription
  if config.as_percent then
    -- Процентное соотношение
    -- Если юзер захотел получать не rps а % текущей метрики от общего rps
    -- применяем эту хитрую логику
    local base = '[0-9]xx$'
    local metricMatcher = '[0-9][0-9x][0-9x]$'
    if type(config.as_percent) == "table" then
      if config.as_percent.base and type(config.as_percent.base) == "string" then
        base = config.as_percent.base
      end
      if config.as_percent.metricMatcher and type(config.as_percent.metricMatcher) == "string" then
        metricMatcher = config.as_percent.metricMatcher
      end
    end

    local key_pattern = key:gsub(metricMatcher, "")
    key_pattern = key_pattern:gsub("[-]", "%-") -- экранируем
    key_pattern = key_pattern:gsub("[.]", "%.") .. base -- экранируем
    log.debug("Compute base for %s by lua pattern %s", key, key_pattern)
    local total = nil
    for k, v in pairs(currentTable) do
      if k:match(key_pattern) and type(v) == "number" then
        total = (total or 0) + v
      end
    end
    if total then
      local prc = value * 100 -- если total почему-то 0, то vlaue 100% равен 100%-ам
      if total > 0 then
        prc = prc / total -- получаем процент value в total
      end
      log.debug(
        "Convert %s=%0.3f to percent from %s=%0.3f: %0.3f",
        key, value, key_pattern, total, prc
      )
      value = prc
      template = triggerDescriptionPrc
    else
      log.error("Can't compute total for %s", key_pattern)
    end
  end
  return value, template
end


-- Если в limits таблица, то она должна быть в форме
-- { WARN: <number>, CRIT: <number>} иначе там должно быть число
function getLevelForThis(limits, val, desc_template, raw_desc, path)
  trigger_threshold = limits
  level = "OK"
  desc_prefix = nil
  if type(limits) == "table" then
    -- Здесь получаем триггер для CRIT, потом для WARN
    local crit_num = limits["CRIT"]
    local warn_num = limits["WARN"]
    if not crit_num or not warn_num then
      log.error("malformed CRIT and WARN limits for %s", path)
      trigger_threshold = nil
    else
      log.debug("limits with CRIT=%0.3f WARN=%0.3f", crit_num, warn_num)
      trigger_threshold = warn_num
      if val > crit_num then
        trigger_threshold = crit_num
        level = "CRIT"
      else
        if val > warn_num then
          level = "WARN"
        end
      end
    end
  else
    -- простой CRIT only лимит
    if trigger_threshold and val > trigger_threshold then
      level = "CRIT"
    end
  end

  -- если почему-то trigger_threshold = nil или false
  if not trigger_threshold then -- если не удалось получить значение лимита ставим дефолтный 0
    log.error("failed to get limits for %s", path)
    desc_prefix = "(failed to get threshold) "
    trigger_threshold = -1
    level = "CRIT"
  end

  desc_prefix = desc_prefix or "("..level..") "
  desc = desc_prefix .. desc_template:format(raw_desc, val, trigger_threshold)
  return level, desc
end

-- Спускается рекурсивно по payload[item] до значений и применяет описанные лимиты
-- на основе этого зажигает CRIT в 'events'
function pick(
    t,    -- t - пока идет рекурсивный спуск 't' - это таблица.
    tags, -- теги для текущего item-а из payload
    pathTable, -- путь до значения 't' в виде списка ключей
    events -- эвенты полученные на выходе, таблица пробрасывается до значений 'v'

  )
  for k, v in pairs(t) do
    table.insert(pathTable, k)
    if type(v) == "table" then
      pick(v, tags, pathTable, events)
    else
      -- тут получаем 'плоский' путь до значения в 'v'
      pathString = table.concat(pathTable, "/")

      local nameOfService, desc = pathString:match(config.query)
      if nameOfService and desc then
        log.debug("work with: service=%s, desc=%s, path=%s", nameOfService, desc, pathString)
        -- если из пути до метрики удалось извлечь имя сервиса и описаине,
        -- значит эту метрику нужно обработать


        -- проверяем есть ли эвент для такого сервиса
        -- ниже к этому событию будет добавлено новое описание.
        aEvent = events[nameOfService] or {desc = {}, level = "OK"}

        local threshold, desc = getThresholdFromSenderConfigs(nameOfService, desc)
        v, template = computePercentage(t, k, v)
        -- добавляем еще одно описание к событию и получаем статус триггера
        level, desc = getLevelForThis(threshold, v, template, desc, pathString)
        if level ~= "OK" then
          log.info("state of %s is %s", pathString, desc)
          table.insert(aEvent.desc, desc)
          aEvent.level = level
        else
          log.info("state of %s is %s", pathString, desc)
        end
        -- возвращаем event в таблицу событий
        events[nameOfService] = aEvent
      else
        log.debug("skip nil match: service=%s, desc=%s, path=%s", nameOfService, desc, pathString)
      end
    end
    -- обработка последнего значения в pathTable
    -- (имя полседнего ключа при рекурсивном спуске) закончена,
    -- нужно очистить префикс хранящийся в pathTable
    table.remove(pathTable) -- удалить последний ключ и pathTable
  end
end

-- про историю событий хранящуюся в media монге.
-- Идея в том, чтобы посылать CRIT только если
-- предыдущие события были N критов подряд и временной интервал
-- между ними укладывается в один интервал TTL проверки
function checkHistory(history, level)
  for idx, event in pairs(history) do
    if event:sub(11, 11) ~= ":" then
      log.error("Garbage event '%s', expected format '<timestamp>:<level>'", event)
      return level
    end
    local event_age = tonumber(event:sub(0, 10))
    local lvl = event:sub(12)
    local allowable_age = os.time() + (checkTTL * idx)
    -- recent events go first
    -- for picker not to record the "OK" event is a valid behavior
    -- so we need check event timestamp,
    -- and consider events only if all events too fresh
    -- otherwise reset level to OK and prevent rare flaps
    if allowable_age < event_age then
      -- premature event reset by ttl checking
      log.info("Reset level to '%s' by too old events: [%s]", table.concat(history, ", "))
      return lvl
    end
    if lvl ~= level then
      log.info("Reset level to '%s' by events_history: [%s]", lvl, table.concat(history, ", "))
      -- pushEvent write any events and
      -- assert lvl == "OK" if checkHistory called only for CRIT events
      return lvl
    end
  end
  return level
end

-- добавляет значение текущего события в медиа монгу
function pushEvent(data, check, level, config)
  if config.history then
    key = "picker:"..data.Tags.metahost..":"..data.Tags.name..":"..check
    log.debug("Push event %s: %s", key, event)
    local event = os.time() ..":".. level
    return events_history(key, event, config.history)
  end
  return nil
end

function run()
  config = config or {}
  config.limits = config.limits or {}
  config.query = config.query or '^([^/]+).*(5xx)$'
  log.debug("Use query: %s", config.query)

  assert(payload)
  if not checkTTL then
    log.error("checkTTL not configured, set default to '90'")
    checkTTL = 90
  end

  evalVariables()   -- now eval variables

  local result = {}
  for _, data in pairs(payload) do
    local events = {}

    if config.type and data.Tags.type ~= config.type then
      log.debug("skip data for '%s-%s' type %s", data.Tags.metahost, data.Tags.name, data.Tags.type)
    else
      pick(data.Result, data.Tags, {}, events)
      for name, service in pairs(events) do
        if #service.desc == 0 then -- если дескрипшен пустой
          service.desc = {"OK"} -- добавим в дескрипшен ОК, don't worry... be happy
        end
        -- если для разных имен сервисов мы хотим разные префиксы...
        -- я вообще слабо верю, что кто-нибудь когда-нибудь узнает про это
        -- но есть очень большая вероятность того, что этим пользуется музыка.
        if config.prefix then
          if type(config.prefix) == "table" then
            prefix = config.prefix[name]
            if prefix then
              name = prefix .. name
            end
          else
            name = config.prefix .. name
          end
        end
        -- Events History push and check
        prevLevels = pushEvent(data, name, service.level, config)
        if prevLevels then
          if service.level == "CRIT" then
            service.level = checkHistory(prevLevels, service.level)
          end
        end
        -- Events History end

        result[#result+1] = {
          tags = data.Tags,
          service = name,
          level = service.level,
          description = table.concat(service.desc, ";"),
        }
      end
    end
  end
  return result
end
