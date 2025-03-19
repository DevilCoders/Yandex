variable "application_version" {
  description = "Version of docker image with ydb-dumper"
  default     = "25472-865a874766"
}

variable "image_id" {
  default = "fd8loccl1l5jp63es9mp"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-lockbox/backup
  default = "b1g978uq2f7dn22rdsrl"
}

variable "service_account_id" {
  // yc-lockbox/backup
  default = "ajedd1s7h2hq1h7chr5b"
}

variable "ycp_profile" {
  default = "prod"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-a"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "2"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  // VLA, MYT
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:500:0:f845:d3bb:0/112
  // SAS: 2a02:6b8:c02:900:0:f845:d3bb:0/112
  // MYT: 2a02:6b8:c03:500:0:f845:d3bb:0/112
  default = ["2a02:6b8:c0e:500:0:f845:d3bb:11", "2a02:6b8:c02:900:0:f845:d3bb:11", "2a02:6b8:c03:500:0:f845:d3bb:11"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.0/16
  // SAS: 172.17.0.0/16
  // MYT: 172.18.0.0/16
  default = ["172.16.0.11", "172.17.0.11", "172.18.0.11"]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9ba19ffma25gqffbmos"
    "ru-central1-b" = "e2ldetr4j3jq08ahuddo"
    "ru-central1-c" = "b0c07vthi8i9fcm4lfmu"
  }
}

variable "host_group" {
  default = "kms"
}

variable "security_group_ids" {
  default = ["enpdn39rq3f4g8rum7pd"]
}

variable "ycp_prod" {
  description = "True if this is a PROD installation"
  default     = "true"
}

variable "hostname_suffix" {
  default = "lockbox.cloud.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v1"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "2"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "4"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "20"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "metadata_image_version" {
  type    = string
  default = "82cc2cb932"
}

variable "push-client_image_version" {
  type    = string
  default = "2020-11-24T12-59"
}
