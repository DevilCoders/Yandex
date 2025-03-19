{
  "/etc/kms/application.yaml": {
    "content": ${jsonencode(application_yaml)}
  },
  "/etc/kms/keys-config.yaml": {
    "content": ${jsonencode(keys_config_yaml)}
  },
  "/etc/kms/master-key-config.yaml": {
    "content": ${jsonencode(master_key_config_yaml)}
  }
}
