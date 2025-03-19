{
  "/etc/certificate-manager/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/certificate-manager/run-tool.sh": {
    "content": ${jsonencode(run_tool_sh)}
  },
  "/etc/certificate-manager/GPNInternalRootCA.crt": {
    "content": ${jsonencode(gpn_internal_root_ca_crt)}
  },
  "/etc/certificate-manager/TechparkCA.crt": {
    "content": ${jsonencode(techpark_ca_crt)}
  }
}