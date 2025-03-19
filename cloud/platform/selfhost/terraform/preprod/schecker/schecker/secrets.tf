
module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = module.common.abc_group
}

# cr.yandex auth token
data ycp_lockbox_secret_reference "lockbox-secret-sa-yc-schecker-cr" {
  sdk_config = "prod"
  secret_id  = "e6q4fjkhblguduabhc4h"
  key        = "docker_key"
}

# schecker db user
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-db-user" {
  sdk_config = "preprod"
  secret_id  = "fc3q15umvag4uq42cg2j"
  key        = "user"
}

# schecker db password
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-db-password" {
  sdk_config = "preprod"
  secret_id  = "fc3q15umvag4uq42cg2j"
  key        = "password"
}

# schecker api token
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-api-token" {
  sdk_config = "preprod"
  secret_id  = "fc39vmj6ss79m458u926"
  key        = "token"
}

# schecker splunk username
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-splunk-username" {
  sdk_config = "prod"
  secret_id  = "e6q07ljq8cid4gi95udk"
  key        = "username"
}

# schecker splunk password
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-splunk-password" {
  sdk_config = "prod"
  secret_id  = "e6q07ljq8cid4gi95udk"
  key        = "password"
}

# schecker startrek oauth token
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-startrek-token" {
  sdk_config = "preprod"
  secret_id  = "fc3u6b64vi41o98vcbe2"
  key        = "oauth_token"
}

# schecker smtp username
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-smtp-user" {
  sdk_config = "prod"
  secret_id  = "e6q7uk6ah8mndu5bl8fq"
  key        = "user"
}

# schecker smtp password
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-smtp-password" {
  sdk_config = "prod"
  secret_id  = "e6q7uk6ah8mndu5bl8fq"
  key        = "password"
}

# schecker clickhouse password
data ycp_lockbox_secret_reference "lockbox-secret-yc-schecker-clickhouse-password" {
  sdk_config = "prod"
  secret_id  = "e6qv07dj6p3ebobb720d"
  key        = "password"
}

data ycp_lockbox_secret_reference "lockbox-logbroker-iam-key" {
  sdk_config = "preprod"
  secret_id  = "fc33g97ol1881hl2vth4"
  key        = "key"
  transform  = "jsonencode"
}

# selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}
