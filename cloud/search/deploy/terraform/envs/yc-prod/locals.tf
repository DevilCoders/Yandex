locals {
  zones = {
    "zone_a" = {
      "letter"    = "k"
      "subnet_id" = "b0cnu3t78idosgg4anhu"
      "id"        = "ru-central1-a"
      "shortname" = "rc1a"
    }
    "zone_b" = {
      "letter"    = "h"
      "subnet_id" = "e2l2sk5audk7g7qb7gf6"
      "id"        = "ru-central1-b"
      "shortname" = "rc1b"
    }
    "zone_c" = {
      "letter"    = "f"
      "subnet_id" = "b0cnu3t78idosgg4anhu"
      "id"        = "ru-central1-c"
      "shortname" = "rc1c"
    }
  }

  vpc_id = "enpqq6umbrmdml18b25f"

  instance_group_variables = {
    ru-central1-a_shortname = local.zones.zone_a.shortname
    ru-central1-b_shortname = local.zones.zone_b.shortname
    ru-central1-c_shortname = local.zones.zone_c.shortname
  }

  env_name = "prod"

  zk_config = {
    "ycsearch-queue-prod01-rc1a.yandexcloud.net" = 1
    "ycsearch-queue-prod01-rc1b.yandexcloud.net" = 2
    "ycsearch-queue-prod01-rc1c.yandexcloud.net" = 3
  }
}
