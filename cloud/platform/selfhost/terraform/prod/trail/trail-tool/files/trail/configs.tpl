{
  "/etc/trail/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/trail/run-tool.sh": {
    "content": ${jsonencode(run_tool_sh)}
  }
}
