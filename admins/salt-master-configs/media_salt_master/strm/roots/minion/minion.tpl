master:
{{ masters|yaml(False) }}
random_master: True
ipv6: True
log_level: info
master_alive_interval: 90
auth_timeout: 2
auth_tries: 3
state_output: changes

recon_default: 50
recon_max: 1000
recon_randomize: True
acceptance_wait_time: 10
random_reauth_delay: 60

pillarenv_from_saltenv: True
saltenv: {{ default_env }}
ignore_init_pillars: True

return:
- strm_report
- rawfile_json
strm_report:
  enable_report: true
  enable_tskv: true
  enable_yasm: true
  per_saltenv:
    {{ default_env }}:
      report_path: /var/log/salt/default_env_report.yaml
  report_ignore_test: true
  report_only_highstate: true
  yasm_tags:
    ctype: {{ ctype }}
    geo: {{ geo }}
    itype: strmsalt
