variable "sender_version" {
  default = "2021-05-20T21-48"
}

##########

variable "image_id" {
  default = "fdvsmkk6aad59lr66td6"
}

##########

variable "metadata_image_version" {
  default = "82cc2cb932"
}

variable "solomon_agent_image_version" {
  default = "2021-04-06T07-32"
}

##########

variable "yc_folder" {
  // yc-osquery/yc-osquery
  default = "aoegc0oie2sg7fugpaoj"
}

variable "service_account_id" {
  // sa-skm-decrypter
  default = "bfb409mps615l40fh1sc"
}

variable "security_group_ids" {
  default = ["c64vkvcdop1s88ojqu48"]
}

variable "yc_instance_group_size" {
  default = "3"
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:fc2e:0:*
  // SAS: 2a02:6b8:c02:901:0:fc2e:0:*
  // MYT: 2a02:6b8:c03:501:0:fc2e:0:*
  default = [
    "2a02:6b8:c0e:501:0:fc2e:0:10",
    "2a02:6b8:c02:901:0:fc2e:0:10",
    "2a02:6b8:c03:501:0:fc2e:0:10"
  ]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = [
    "172.16.0.10",
    "172.17.0.10",
    "172.18.0.10"
  ]
}

variable "subnets" {
  default = {
    "ru-central1-a" = "buchpdeuf1olhkut49p1"
    "ru-central1-b" = "bltcqd1e7cehnkgt9r4g"
    "ru-central1-c" = "fo2hlieqbllveb19bu9e"
  }
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "8"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "25"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-hdd"
}

variable "lb_address" {
  default = "2a0d:d6c0:0:ff1b::1d9"
}

##########

variable "host_group" {
  default = "service"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "hostname_suffix" {
  default = "osquery.cloud-preprod.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "ycp_profile" {
  default = "preprod"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-c"
}

variable "yc_region" {
  default = "ru-central1"
}

variable "abc_group" {
  default = "ycosquery"
}

variable "sender_registry" {
//  default = "crplcdf04h2mbhru5ah3"
  default = "crprkf5jev260ua4o8tn"
}

##########

variable "sender_memory_limit" {
  default = "4096Mi"
}

variable "ch_sender_batch_memory" {
  default = "512MB"
}

variable "s3_sender_batch_memory" {
  default = "512MB"
}

variable "solomon_memory_limit" {
  default = "256Mi"
}

variable "solomon_storage_limit" {
  default = "128MiB"
}

variable "clickhouse_hosts" {
  default = "['rc1b-ykjyd4e9f22rlfei.mdb.cloud-preprod.yandex.net', 'rc1c-zzu89m5e2dbxv3ds.mdb.cloud-preprod.yandex.net']"
}

variable "s3_endpoint" {
  default = "storage.cloud-preprod.yandex.net"
}

variable "s3_access_key_id" {
  default = "URBZSrBKeEECvr84RLfc"
}

##########

variable "api_port" {
  default = 443
}

variable "healthcheck_port" {
  default = 444
}

##########

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

##########

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
