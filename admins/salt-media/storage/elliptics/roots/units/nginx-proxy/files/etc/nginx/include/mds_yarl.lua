local function update_yarl ()
    local plugin = require("yarl/yarl-go")
    local prior = ngx.var.arg_prior
    local namespace = ngx.var.unistat_namespace
    local request_type = ngx.var.unistat_request_type
    local quota_type = ''
    {% if grains['yandex-environment'] == 'testing' -%}
    local env = 'testing'
    {%- else -%}
    local env = 'production'
    {%- endif %}
    if prior ~= nil and prior ~= '' and prior ~= 0 then
        ngx.var.unistat_prior = 1
        quota_type = 'meta'
    else
        ngx.var.unistat_prior = 0
        quota_type = 'low'
    end
    -- mds:curiosity-karl-only:{get, put, etc}:{meta, low}:{testing, production}
    local quota = string.format("mds:%s:%s:%s:%s", namespace, request_type, quota_type, env)
    plugin.limit_by_unique_name(quota, 1)
end

local status, err = pcall(update_yarl)
if not status then
    increment_metric('stat_yarl_error_dmmm', 1)
    ngx.log(ngx.ERR, string.format("Limit yarl status: %s. Err: %s", tostring(status), tostring(err)))
end

