local S3Internal        = require "s3.internal"
local S3Log             = require "s3.log"
local S3Files           = require "s3.files"
local S3PrivateAPI      = require "s3.private-api"

local name_dict       = S3Internal.require_dict("s3_open_buckets")
local regex_dict      = S3Internal.require_dict("s3_open_buckets_regex")
local file_cache_path = "/var/tmp/nginx/open_buckets.json"

-- Bit flags of open bucket access settings.
-- Nginx shared dicts for LUA can't store complex data structures.
-- They work only with scalars, that's why we have to use bit flags instead
-- of regular LUA table (which will be more convenient).
local flags = {
    OPEN_PUBLIC     = 1, -- ...000000001
    OPEN_PRIVATE    = 2, -- ...000000010
    OPEN_STAFF      = 3, -- ...000000011
    OPEN_ZEN        = 4, -- ...000000100
}

local regex_cache             = {} -- local worker cache with regex rules, allows to avoid frequent :get_keys(0) calls
local regex_cache_last_update = 0  -- last update time of local worker regex settings cache
local regex_cache_ttl         = 30 -- time to keep regex settings in local worker memory

local OpenBuckets = {}

--- Try to refresh buckets external access settings cache file with fresh data loaded from Private API
--- @param data table - fresh data from S3 Private API
local function refreshFileCache(data)
    local err = S3Files.write_json(file_cache_path, data)
    if err then
        S3Log.Error("failed to update S3 open buckets access settings cache: " .. tostring(err))
        return
    end

    S3Log.Info("updated S3 open buckets access settings cache file '" .. file_cache_path .. "'")
end

local function isPattern(api_bucket_settings)
    return api_bucket_settings["is_pattern"]
end

local function updateBucketSettings(api_bucket_settings, ttl)
    local bucket_name = api_bucket_settings["name"]
    local access_settings = 0;

    if api_bucket_settings["open_public"] then
        access_settings = bit.bor(access_settings, flags.OPEN_PUBLIC)
    end
    if api_bucket_settings["open_private"] then
        access_settings = bit.bor(access_settings, flags.OPEN_PRIVATE)
    end
    if api_bucket_settings["open_staff"] then
        access_settings = bit.bor(access_settings, flags.OPEN_STAFF)
    end
    if api_bucket_settings["open_zen"] then
        access_settings = bit.bor(access_settings, flags.OPEN_ZEN)
    end

    if isPattern(api_bucket_settings) then
        -- 'get_keys' is expensive blocking operation on large shared dicts.
        -- This is why we put regular expressions into separate dicts to keep
        -- them as small as possible.
        -- We also want to be sure the records will not disappear from dict
        -- if periodic sync get broken (ttl = 0).
        regex_dict:set(bucket_name, access_settings, 0)
    else
        name_dict:set(bucket_name, access_settings, ttl)
    end
end

--- :get_keys(0) is expensive, there is no need to call it for each checked request:
--- we know it is rarely updated by periodic sync process and we can safely cache results
--- in local worker memory for short period of time.
--- This dramatically reduces amount of :get_keys(0) calls under high load
local function updateWorkerCache()
    local cache_age = ngx.time() - regex_cache_last_update
    if cache_age < regex_cache_ttl then
        return
    end

    local regexes = regex_dict:get_keys(0)

    local cache = {}
    for _, regex in ipairs(regexes) do
        cache[regex] = regex_dict:get_stale(regex)
    end

    S3Log.Info("Cache with open bucket regex settings was updated in worker '" .. tostring(ngx.worker.pid()) .. "'")
    regex_cache             = cache
    regex_cache_last_update = ngx.time()
end

--- Remove old data from shared memory.
--- @param actual_regexes table - list of actual regex settings from latest API response.
---                               Since we can't use regular 'flush' on regex_dict, we have to manually detect
---                               old records and drop them.
local function clearOldRecords(actual_regexes)
    name_dict:flush_expired()

    local shared_regexes = regex_dict:get_keys(0)
    for _, regex in ipairs(shared_regexes) do
        if not actual_regexes[regex] then
            regex_dict:delete(regex)
        end
    end

    S3Log.Info("removed outdated open bucket access settings from nginx shared memory")
end

--- Get external access settings for bucket
--- @param bucket_name string - bucket to check
--- @return number - bitmap of open bucket access settings. Use OpenBuckets.flags.* to check particular flag values
local function getAccessSettings(bucket_name)
    local bucket_settings = name_dict:get_stale(bucket_name)
    if bucket_settings then
        S3Log.Info("found open bucket settings for bucket '" .. bucket_name .. "' by direct name")
        return bucket_settings
    end

    -- No direct name match. Check all regular expressions
    updateWorkerCache()

    for regex, re_bucket_settings in pairs(regex_cache) do
        if ngx.re.match(bucket_name, regex) then
            S3Log.Info("found open bucket settings for bucket '" .. bucket_name .. "' by regex '" .. regex .. "'")
            return re_bucket_settings
        end
    end

    S3Log.Info("no open bucket settings found for bucket '" .. bucket_name .. "'")
    return 0
end

--- Check if bitmap has bit enabled
--- @param bitmap number
--- @param flag number
--- @return boolean
local function checkFlag(bitmap, flag)
    return bit.band(bitmap, flag) > 0
end

function OpenBuckets.refresh(ttl)
    S3Log.Info("updating S3 open buckets access settings")

    local client = S3PrivateAPI.new()
    local buckets, err = client:buckets_opened()

    if err then
        S3Log.Error("failed to load S3 open buckets access settings from API: " .. err)

        buckets, err = S3Files.readall_json(file_cache_path)
        if err then
            S3Log.Error("failed to get list of buckets from local cache: " .. err .. ". Data sync interrupted for safety reasons.")
            return
        end

        if #buckets == 0 then
            S3Log.Error("empty S3 open buckets access settings loaded from cache. Data sync interrupted for safety reasons.")
            return
        end

        S3Log.Info("loaded S3 open buckets access settings from cache")
    else
        if #buckets == 0 then
            S3Log.Error("empty S3 open buckets access settings loaded from API. Data sync interrupted for safety reasons.")
            return
        end

        S3Log.Info("loaded S3 open buckets access settings from API")
        refreshFileCache(buckets)
    end

    -- Add buckets listed in response
    local patterns = {}
    for _, settings in ipairs(buckets) do
        local name = settings["name"]
        if name == "" then
            S3Log.Error("broken bucket info, empty bucket name detected. Data sync interrupted for safety reasons")
            return
        end

        updateBucketSettings(settings, ttl)

        if isPattern(settings) then
            patterns[name] = true
        end
    end

    S3Log.Info("updated shared dictionaries with fresh open bucket access settings")
    clearOldRecords(patterns)
end

--- Check if bucket is public
--- @param bucket_name string - bucket to check
--- @return boolean
function OpenBuckets.isPublicAccessAllowed(bucket_name)
    local access_settings = getAccessSettings(bucket_name)

    if checkFlag(access_settings, flags.OPEN_PUBLIC) then
        S3Log.Info("public access is allowed for bucket '" .. bucket_name .. "'")
        return true
    end

    S3Log.Info("public access denied for bucket '" .. bucket_name .. "'")
    return false
end

--- Check if bucket is public
--- @param bucket_name string - bucket to check
--- @return boolean
function OpenBuckets.isPrivateAccessAllowed(bucket_name)
    local access_settings = getAccessSettings(bucket_name)

    if checkFlag(access_settings, flags.OPEN_PRIVATE) then
        S3Log.Info("private access is allowed for bucket '" .. bucket_name .. "'")
        return true
    end

    S3Log.Info("private access denied for bucket '" .. bucket_name .. "'")
    return false
end

--- Check if bucket is open for staff
--- @param bucket_name string - bucket to check
--- @return boolean
function OpenBuckets.isStaffAccessAllowed(bucket_name)
    local access_settings = getAccessSettings(bucket_name)

    if checkFlag(access_settings, flags.OPEN_STAFF) then
        S3Log.Info("staff access is allowed for bucket '" .. bucket_name .. "'")
        return true
    end

    S3Log.Info("staff access denied for bucket '" .. bucket_name .. "'")
    return false
end

--- Check if bucket is open for zen
--- @param bucket_name string - bucket to check
--- @return boolean
function OpenBuckets.isZenAccessAllowed(bucket_name)
    local access_settings = getAccessSettings(bucket_name)

    if checkFlag(access_settings, flags.OPEN_ZEN) then
        S3Log.Info("zen access is allowed for bucket '" .. bucket_name .. "'")
        return true
    end

    S3Log.Info("zen access denied for bucket '" .. bucket_name .. "'")
    return false
end


return OpenBuckets
