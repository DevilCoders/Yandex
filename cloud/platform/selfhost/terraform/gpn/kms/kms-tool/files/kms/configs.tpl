{
  "/etc/kms/application.yaml": {
    "content": ${jsonencode(application_yaml)}
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