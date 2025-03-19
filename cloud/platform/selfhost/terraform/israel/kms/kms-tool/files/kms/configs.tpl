{
  "/etc/kms/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/kms/ssl/certs/allCAs.pem": {
    "content": ${jsonencode(yandex_internal_root_ca)}
  },
  "/usr/local/bin/run-tool.sh": {
    "content": ${jsonencode(run_tool_sh)},
    "mode": "755"
  },
  "/usr/local/bin/run-ydb.sh": {
    "content": ${jsonencode(run_ydb_sh)},
    "mode": "755"
  }
}
