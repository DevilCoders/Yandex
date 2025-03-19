{% from slspath ~ "/macro.sls" import restart_if_needed with context %}

# Note: this also restarts ifmap because of dependency in systemd
{{ restart_if_needed('contrail-api') }}

