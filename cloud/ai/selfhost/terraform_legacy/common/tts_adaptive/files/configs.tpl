{
  "/etc/solomon-agent/agent.conf": {
    "content": ${jsonencode(solomon_agent_conf)}
  },
  "/etc/solomon-agent/solomon-agent-extra.conf": {
    "content": ${jsonencode(solomon_agent_extra_conf)}
  },
  "/etc/logrotate.d/logrotate-app.conf": {
    "content": ${jsonencode(logrotate_app_conf)}
  }
}