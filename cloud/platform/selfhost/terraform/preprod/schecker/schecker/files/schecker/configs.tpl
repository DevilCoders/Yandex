{
  "/opt/schecker/schecker-swiss-knife/config.yaml": {
    "content": ${jsonencode(swiss_knife_config_yaml)}
  },
  "/opt/schecker/schecker-swiss-knife/groups.config.yaml": {
    "content": ${jsonencode(groups_swiss_knife_config_yaml)}
  },
  "/opt/schecker/syncer/config.yaml": {
    "content": ${jsonencode(syncer_config_yaml)}
  },
  "/opt/schecker/syncer/groups.config.yaml": {
    "content": ${jsonencode(groups_syncer_config_yaml)}
  },
  "/opt/schecker/parser/config.yaml": {
    "content": ${jsonencode(parser_config_yaml)}
  }
}
