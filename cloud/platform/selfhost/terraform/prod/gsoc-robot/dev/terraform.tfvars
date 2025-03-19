##########

image_id = "fd8eoc8p9ipsgb57mqlj" // paas-base-g4

cluster = "prod"

########## Compute config

yc_folder = "b1gn4f32jfa3mv8kk4eo" // yc-osquery/dev

service_account_id = "aje8l8m8af7237jfe2ad" // gsoc-robot-sa

security_group_ids = ["enp0bh7ko426f233p5tp"]

instances_amount = "1"

// VLA: 2a02:6b8:c0e:501:0:fc2e:0:*
// SAS: 2a02:6b8:c02:901:0:fc2e:0:*
// MYT: 2a02:6b8:c03:501:0:fc2e:0:*
ipv6_addresses = [
    "2a02:6b8:c0e:500:0:f84d:0:40",
    "2a02:6b8:c02:900:0:f84d:0:40",
    "2a02:6b8:c03:500:0:f84d:0:40"
  ]

// VLA: 172.16.0.*
// SAS: 172.17.0.*
// MYT: 172.18.0.*
ipv4_addresses = [
    "172.16.0.40",
    "172.17.0.40",
    "172.18.0.40"
  ]

subnets = {
    "ru-central1-a" = "e9bk341d4darmppkshmr"
    "ru-central1-b" = "e2lt4oso7hr3k96dn0g3"
    "ru-central1-c" = "b0c2chccaes3bsap9am1"
  }

platform_id = "standard-v2"
instance_cores = "4"
instance_memory = "8"
instance_disk_size = "25"
instance_disk_type = "network-ssd"

labels = {
  env     = "prod"
  layer   = "iaas"
  abc_svc = "ycosquery"
}

########## Application config

database = {
  host = "rc1a-e34htoou4my7xss2.mdb.yandexcloud.net"
  port = "6432"
  name = "gsoc-robot-db"
  user = "gsoc-robot-user"
}
smtp = {
  addr = "smtp.yandex-team.ru:465"
  user = "robot-yc-sec@yandex-team.ru"
}

kinesis = {
  endpoint = "https://yds.serverless.yandexcloud.net"
  region = "ru-central-1"
  stream_name = "/ru-central1/b1gmrhakmf7ar1i6l6f8/etngu1bpm9k7sgtpq6s7/hids"
  key_id = "tBNd0oMt_ReQe0kXm50J"
}

########## Other

host_group = "service"

yc_zones = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]

yc_zone_suffixes = {
  ru-central1-a = "vla"
  ru-central1-b = "sas"
  ru-central1-c = "myt"
}

dns_zone = "gsoc-robot.cloud.yandex.net"

instance_platform_id = "standard-v2"

ycp_profile = "prod"
ycp_zone = "ru-central1-c"

abc_group = "ycosquery"

solomon_storage_limit = "256MiB"

##########
