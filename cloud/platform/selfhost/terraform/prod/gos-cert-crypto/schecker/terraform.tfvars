##########

image_id = "fd8r32gg1h5iehijsdep" // osquery-sender base image 

cluster = "prod"

########## Compute config

yc_folder = "yc.goscertcrypto.folder" // yc.goscertcrypto.cloud/yc.goscertcrypto.folder

security_group_ids = ["enp1n24qftgokj3g66ht"]

instances_amount = "1"

// VLA: 2a02:6b8:c0e:501:0:f8a1:66db:*
// SAS: 2a02:6b8:c02:901:0:f8a1:66db:*
// MYT: 2a02:6b8:c03:501:0:f8a1:66db:*
ipv6_addresses = [
    "2a02:6b8:c0e:500:0:f8a1:66db:50",
    "2a02:6b8:c02:900:0:f8a1:66db:50",
    "2a02:6b8:c03:500:0:f8a1:66db:50"
  ]

// VLA: 172.16.0.*
// SAS: 172.17.0.*
// MYT: 172.18.0.*
ipv4_addresses = [
    "172.16.0.50",
    "172.17.0.50",
    "172.18.0.50"
  ]

subnets = {
    "ru-central1-a" = "e9brl6lldmg314ehbbf0"
    "ru-central1-b" = "e2l6ftia6ccbel7ehm1v"
    "ru-central1-c" = "b0cm7cd52l9k5sibnd8f"
  }

platform_id = "standard-v2"
instance_cores = "4"
instance_memory = "8"
instance_disk_size = "50"
instance_disk_type = "network-ssd"

labels = {
  env     = "prod"
  layer   = "iaas"
  abc_svc = "ycosquery"
}

########## Application config

database = {
  host = "rc1b-bgbh206oji2iyrt5.mdb.yandexcloud.net"
  port = "6432"
  name = "schecker-db"
  user = "schecker"
}
smtp = {
  addr = "smtp.yandex-team.ru:465"
  user = "robot-yc-sec@yandex-team.ru"
}

########## Other

host_group = "service"

yc_zones = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]

yc_zone_suffixes = {
  ru-central1-a = "vla"
  ru-central1-b = "sas"
  ru-central1-c = "myt"
}

dns_prefix = "schecker"
dns_zone = "crypto.goscert.cloud.yandex.net"
dns_zone_id = "dnsb5f9f73u2jt34uu0c"

instance_platform_id = "standard-v2"

ycp_profile = "prod"
ycp_zone = "ru-central1-c"

abc_group = "ycosquery"

solomon_storage_limit = "256MiB"

##########
