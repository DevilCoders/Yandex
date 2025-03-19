# MANAGED BY SALT
#{{yaenv}} .env.{{yaenv}}
export SYMFONY_ENV={{yaenv}}
export SYMFONY_DEBUG={{debug}}
{% if tvmtoken is defined -%}
export QLOUD_TVM_TOKEN={{tvmtoken}}
{% endif -%}
{% if tvm_port is defined -%}
export TVMTOOL_LOCAL_PORT={{tvm_port}}
{% endif -%}
{% if js_api_key is defined -%}
export YANDEX_MAPS_JS_API_KEY={{js_api_key}}
{% endif -%}
{% if yp_token is defined -%}
export YP_TOKEN={{yp_token}}
{% endif -%}
{% if yaenv == "development" -%}
export SYMFONY__ENV_PREFIX={{dev_user}}
{% endif -%}
{% if redis_cache_hosts is defined -%}
export SYMFONY_REDIS_CACHE_SENTINELS={{redis_cache_hosts}}
export SYMFONY_REDIS_CACHE_READ_HOST="tcp://localhost:16379"
{% endif -%}

{% for section, secrets in data.items() %}
# {{section}}
{%- for key, val in secrets.items() %}
export {{key}}="{{val}}"
{%- endfor %}
{% endfor %}
