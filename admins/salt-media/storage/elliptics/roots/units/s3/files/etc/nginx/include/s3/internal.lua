local Signal    = require "signal"
local S3Log     = require "s3.log"

local S3Internal = {}

-- Map virtual bucket names to real ones.
--    E.g. bucket name in request app-12345.games.s3.yandex.net (virtual bucket 'app-12345.games') actually
--    is routed to games.s3.yandex.net (bucket 'games')
local bucket_aliases = {
    ['^[\\p{L}\\p{N}-]+[.]games$']              = 'games',
}

-- Map real bucket names to virtual quotas to keep several buckets covered by single quota hierarchy
--    E.g. 'internal-dbaas-mdbta7tmoo4u83is72ej' and 'internal-dbaas-mdb65ortljm74oh9a6d5' can both be
--    covered by bucket quota 'internal-dbaas'
local bucket_quota_aliases = {
    ['^internal-dbaas-[\\p{L}\\p{N}-]+$']       = 'internal-dbaas',
    ['^yandexcloud-dbaas-[\\p{L}\\p{N}-]+$']    = 'yandexcloud-dbaas',
    ['^s3-test-[\\p{L}\\p{N}-]+$']              = 's3-test',
}

-- Map special FQDNs to bucket names.
--   This allows us to selectively support domains outside of common S3 (s3.mds.yandex.net and so on)
local special_fqdns = {
    ['^pdamts-content[.]adfox[.]ru$']           = 'adfox-content',
    ['^pdamts[.]adfox[.]ru$']                   = 'adfox-content',
    ['^content[.]adfox[.]ru$']                  = 'adfox-content',
    ['^banners[.]adfox[.]ru$']                  = 'adfox-content',

    ['^pages[.]browser[.]yandex[.]ru$']         = 'browser-pages',
    ['^pano[.]maps[.]yandex[.]net$']            = 'pano',
}

--- Check if string matches regular expression.
---
---@param str string
---@param re string
---@return boolean - true if <str> contents match <re>
local function matchRE(str, re)
    local from = ngx.re.find(str, re, 'os')
    return from ~= nil and from >= 1
end

--- If <str> matches one of alias rules - return an alias.
--- Return unmodified <str> if it does not match any of alias rules.
---
---@param str string
---@param alias_rules table
---@return string, boolean - 1. result: original <str> value or alias for it
---                          2. is_alias: 'true' when <result> is alias for <str>,
---                                       'false' when <result> is original <str> value
local function alias(str, alias_rules)
    for pattern, alias_value in pairs(alias_rules) do
        if matchRE(str, pattern) then
            return alias_value, true
        end
    end

    return str, false
end

--- Extract bucket name from request URI if it looks like 'that kind of request'
---
---@param uri string - sanitized request URI
---@return string - bucket name extracted from URI or empty string when bucket name not found
local function bucketNameFromUri(uri)
    -- TODO: replace regex with more strict version, e.g. ^/([a-z0-9][a-z0-9.-]*[a-z0-9])
    --       from Amazon S3 docs:
    --        - bucket names must be between 3 and 63 characters long.
    --        - bucket names can consist only of lowercase letters, numbers, dots (.), and hyphens (-).
    --        - bucket names must begin and end with a letter or number.
    local bucket = string.match(uri, "^/+([^/?]+)")
    if bucket == nil then
        bucket = ""
    end
    return bucket
end

--- Extract bucket name from FQDN when it looks like 'that kind of request'
---
---@param fqdn string - actual request FQDN
---@param pattern string - pattern for FQDN check and trim, e.g. '[.]s3[.]yandex[.]net$'
---@return string - bucket name extracted from FQDN or empty string when no bucket name provided in 'Host:' HTTP header
local function bucketNameFromDomain(fqdn, pattern)
    if string.find(fqdn, pattern) then
        return string.gsub(fqdn, pattern, '')
    end

    return ""
end

--- Get bucket name from request FQDN and URI
--- @param fqdn string
--- @param uri string
--- @return string, boolean - <bucket name>, <is external>
local function getBucketName(fqdn, uri)
    local bucket

    if fqdn == 's3.mds.yandex.net'
        or fqdn == 's3-zen.mds.yandex.net'
        or fqdn == 's3-website.mds.yandex.net'
        or fqdn == 's3.mdst.yandex.net'
        or fqdn == 's3-zen.mdst.yandex.net'
        or fqdn == 's3-website.mdst.yandex.net'
    then
        bucket = bucketNameFromUri(uri)
        return bucket, false
    end

    bucket = bucketNameFromDomain(fqdn, '[.]s3[-]private[.]mdst?[.]yandex[.]net$')
    if bucket ~= "" then
        -- <bucket> will be empty here for path addressing style requests (s3-private.mds.yandex.net/<bucket_name>/...).
        -- This forbids all path addressing style requests to buckets with 'open_private' attribute.
        -- We intentionally require only vhost addressing style for such buckets.
        return bucket, true
    end

    bucket = bucketNameFromDomain(fqdn, '[.]s3[.]yandex[.]net$')
    if bucket ~= "" then
        -- <bucket> will be empty here for path addressing style requests (s3.yandex.net/<bucket_name>/...).
        -- This forbids all path addressing style requests to buckets with 'open_public' attribute.
        -- We intentionally require only vhost addressing style for such buckets.
        return bucket, true
    end

    bucket = bucketNameFromDomain(fqdn, '[.]s3[.]mdst?[.]yandex[.]net$')
    if bucket ~= "" then
        -- <bucket> will be empty here for path addressing style requests (s3.mds.yandex.net/<bucket_name>/...).
        -- This forbids all path addressing style requests to buckets with 'open_public' attribute.
        -- We intentionally require only vhost addressing style for such buckets.
        return bucket, false
    end

    bucket = bucketNameFromDomain(fqdn, '[.]s3-zen[.]mdst?[.]yandex[.]net$')
    if bucket ~= "" then
        -- <bucket> will be empty here for path addressing style requests (s3-zen.mds.yandex.net/<bucket_name>/...).
        -- This forbids all path addressing style requests to buckets with 'open_public' attribute.
        -- We intentionally require only vhost addressing style for such buckets.
        return bucket, false
    end

    bucket = bucketNameFromDomain(fqdn, '[.]s3-website[.]mdst?[.]yandex[.]net$')
    if bucket ~= "" then
        -- <bucket> will be empty here for path addressing style requests (s3.yandex.net/<bucket_name>/...).
        -- This forbids all path addressing style requests to buckets with 'open_public' attribute.
        -- We intentionally require only vhost addressing style for such buckets.
        return bucket, false
    end

    bucket = bucketNameFromDomain(fqdn, '[.]s3-staff[.]mdst?[.]yandex[.]net$')
    if bucket ~= "" then
        -- <bucket> will be empty here for path addressing style requests (s3.yandex.net/<bucket_name>/...).
        -- This forbids all path addressing style requests to buckets with 'open_public' attribute.
        -- We intentionally require only vhost addressing style for such buckets.
        return bucket, false
    end

    -- TODO: replace regex with more strict version, e.g. ^([a-z0-9][a-z0-9-]*[a-z0-9])
    return string.match(fqdn, "^([^.]+)"), false
end

--- Get bucket name of current nginx request
--- @return string, boolean, boolean - <bucket name>, <is external>
function S3Internal.getBucketName()
    local host = ngx.var.host
    local method = ngx.var.request_method
    local bucket
    local matched
    local external

    -- Handle special domains first
    bucket, matched = alias(host, special_fqdns)
    if matched then
        return bucket, true
    end

    if method == "CONNECT" then
        host = "unknown.s3.mds.yandex.net"
    end
    if host == nil then
        host = "unknown.s3.mds.yandex.net"
    end
    bucket, external = getBucketName(host, ngx.var.request_uri)

    -- Perform special magic to route several domains into single real bucket
    bucket = alias(bucket, bucket_aliases)

    return bucket, external
end

function S3Internal.getYarlQuotaName(env, limit_type, bucket_name, action_type, priority)
    -- Perform special magic to control several buckets by single quota hierarchy
    bucket_name = alias(bucket_name, bucket_quota_aliases)

    local quota_type = env .. ":" .. limit_type
    local instance = "b:" .. bucket_name

    local action
    if priority == "" then
        action = "nginx-control:" .. action_type
    else
        action = "nginx-control:" .. action_type .. ":" .. priority
    end

    -- 's3_<env>:<limit_type>_b:<bucket_name>_nginx-control:<read|write>[:<priority>]'
    return "s3_" .. quota_type .. "_" .. instance .. "_" .. action
end

--- Kill current nginx worker with SIGKILL.
--- When done during initialization process (e.g. from init_worker_by_lua*)
--- it affects even 'nginx -t' command and prevents nginx from loading with broken lua code
function S3Internal.break_nginx(message)
    S3Log.Emergency(message)
    Signal.kill(ngx.worker.pid(), "KILL")
end

--- Require lua shared dict to be declared in nginx configuration.
---
--- Use this method at nginx initialization steps to prevent nginx from
--- passing configuration test and loading: init_by_lua*, init_worker_by_lua*, etc.
function S3Internal.require_dict(name)
    local dict = ngx.shared[name]
    if not dict then
        S3Internal.break_nginx("'".. name .. "' shared dict is not declared in nginx configuration")
    end

    return dict
end

return S3Internal
