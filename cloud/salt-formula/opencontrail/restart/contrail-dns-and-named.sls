{% from slspath ~ "/macro.sls" import restart_if_needed with context %}

# Note: this also restarts named!
{{ restart_if_needed('contrail-dns') }}
