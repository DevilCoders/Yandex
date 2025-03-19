
module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = module.common.abc_group
}

# cr.yandex auth token
module "lockbox-secret-sa-yc-schecker-cr" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6q4fjkhblguduabhc4h"
  value_key  = "docker_key"
}

# schecker db user
module "lockbox-secret-yc-schecker-db-user" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6qs1jc0tbbkud0dqeee"
  value_key  = "user"
}

# schecker db password
module "lockbox-secret-yc-schecker-db-password" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6qs1jc0tbbkud0dqeee"
  value_key  = "password"
}

# schecker api token
module "lockbox-secret-yc-schecker-api-token" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6q7o52gplcdroc1ilq2"
  value_key  = "token"
}

# schecker splunk username
module "lockbox-secret-yc-schecker-splunk-username" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6q07ljq8cid4gi95udk"
  value_key  = "username"
}

# schecker splunk password
module "lockbox-secret-yc-schecker-splunk-password" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6q07ljq8cid4gi95udk"
  value_key  = "password"
}

# schecker startrek oauth token
module "lockbox-secret-yc-schecker-startrek-token" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6qhbvn38j563e0g7hom"
  value_key  = "oauth_token"
}

# schecker smtp username
module "lockbox-secret-yc-schecker-smtp-user" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6q7uk6ah8mndu5bl8fq"
  value_key  = "user"
}

# schecker smtp password
module "lockbox-secret-yc-schecker-smtp-password" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6q7uk6ah8mndu5bl8fq"
  value_key  = "password"
}

# schecker clickhouse password
module "lockbox-secret-yc-schecker-clickhouse-password" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6qv07dj6p3ebobb720d"
  value_key  = "password"
}

module "lockbox-logbroker-iam-key" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6qashqjvho28ebshqua"
  value_key = "key"
}

# selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}
