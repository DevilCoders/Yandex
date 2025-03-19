repos:
  sources:
    - libstd++:
      - ppa: ubuntu-toolchain-r/test

{% set tvmtool_sec_id = 'sec-01dfbgthf0f614c38dw9ebzdxn' %}
tvmtool_id: {{ salt.yav.get(tvmtool_sec_id+"[id]") }}
tvmtool_secret: {{ salt.yav.get(tvmtool_sec_id+"[secret]") | json }}
