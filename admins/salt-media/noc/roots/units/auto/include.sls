{%- set incs = salt["pillar.get"]("salt-autodeploy-sls") %}
include:
  - .self
  {%- for sls_name in incs.keys() %}
    {%- do salt.log.info('Include sls {!r}'.format(sls_name)) %}
  - {{sls_name}}
  {%- endfor %}
