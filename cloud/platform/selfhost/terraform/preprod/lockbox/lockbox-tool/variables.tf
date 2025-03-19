variable "application_version" {
  description = "Version of docker image with lockbox-tool"
  default     = "26220-dfe20c75e7"
}

variable "image_id" {
  default = "fdv5oi8niblta6smb045"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default = "aoe2fnrknrpgsho3chc5"
}

variable "service_account_id" {
  // yc-lockbox/lockbox
  default = "bfbc91a56dngtkj4epph"
}

variable "ycp_profile" {
  default = "preprod"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1-a"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  // VLA, MYT
  default = ["ru-central1-a"]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "buc91ifa8c8cdec85vr7"
  }
}

variable "host_group" {
  default = "service"
}

variable "security_group_ids" {
  default = ["c641egpce17hsqrosvr3"]
}

variable "ycp_prod" {
  description = "True if this is a PROD installation"
  default     = "false"
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = ["172.16.0.251"]
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f810:0:*
  // SAS: 2a02:6b8:c02:901:0:f806:0:*
  // MYT: 2a02:6b8:c03:501:0:f810:0:*
  default = ["2a02:6b8:c0e:501:0:fc26:0:301"]
}

variable "hostname_suffix" {
  default = "lockbox.cloud-preprod.yandex.net"
}

variable "instance_platform_id" {
  default = "standard-v2"
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
  default = "2021-02-03T14-09"
}
