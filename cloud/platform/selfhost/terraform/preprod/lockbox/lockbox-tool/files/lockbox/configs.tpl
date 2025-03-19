{
 "/etc/lockbox/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
 "/usr/local/bin/run-tool.sh": {
   "content": ${jsonencode(run_tool_sh)},
   "mode": "755"
 }
}
