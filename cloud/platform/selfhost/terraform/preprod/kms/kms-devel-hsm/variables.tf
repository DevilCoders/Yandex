variable "image_id" {
  default = "fdv9p79mvdg2r5tnsi89"
}

////////

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms-devel/default
  default = "aoeosqhpeq0gtc22770o"
}

variable "service_account_id" {
  // yc-kms-devel/sa-kms-devel-hsm
  default = "bfba3trlppua108bdqja"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-a"
}

variable "host_group" {
  default = "service"
}

variable "security_group_ids" {
  default = []
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  // VLA, SAS, MYT
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:fc38:6a80:0/112
  // SAS: 2a02:6b8:c02:901:0:fc38:6a80:0/112
  // MYT: 2a02:6b8:c03:501:0:fc38:6a80:0/112
  default = ["2a02:6b8:c0e:501:0:fc38:6a80:101"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.0/16
  // SAS: 172.17.0.0/16
  // MYT: 172.18.0.0/16
  default = ["172.16.0.101"]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "buc6pv2juqmb8e76vres"
    "ru-central1-b" = "bltgrd6441qs70kd74cm"
    "ru-central1-c" = "fo29f73thb7bhvkd3d79"
  }
}

variable "ycp_prod" {
  description = "Is this Yandex Cloud PROD?"
  default     = false
}

variable "hostname_suffix" {
  default = "kms.cloud-preprod.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "4"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "16"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "30"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}
