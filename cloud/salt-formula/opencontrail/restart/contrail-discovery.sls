{% from slspath ~ "/macro.sls" import restart_if_needed with context %}
{{ restart_if_needed('contrail-discovery') }}
