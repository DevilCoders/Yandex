{
  "/etc/certificate-manager/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/usr/local/bin/run-ydb-dumper.sh": {
    "content": ${jsonencode(run_ydb_dumper_sh)},
    "mode": "755"
  }
}
