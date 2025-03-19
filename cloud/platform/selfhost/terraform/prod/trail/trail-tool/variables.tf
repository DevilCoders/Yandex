variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-cloud-trail/default
  default = "b1gtrvd4ntcgpob8ga7r"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "ru-central1"
}

variable "service_account_id" {
  // yc-cloud-trail/sa-trail-tool
  default = "ajefsg6khvgein7co5mu"
}

variable "image_id" {
  default = "fd8bbmmkfe3k2j9ggtbg"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "1"
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"

  // VLA
  default = ["ru-central1-a"]
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:500:0:f837:ebf2:*
  // PROD IPs start with 120
  default = ["2a02:6b8:c0e:500:0:f837:ebf2:122"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // PROD IPs start with 120
  default = ["172.16.0.122"]
}

variable "subnets" {
  type = "map"

  default = {
    "ru-central1-a" = "e9beh1u93kbb9kkb340d"
    "ru-central1-b" = "e2l9ohqt4eaf1mre6puj"
    "ru-central1-c" = "b0cbr1gsb85d5dibp3e1"
  }
}

variable "yc_zone_suffix" {
  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

variable "hostname_suffix" {
  default = "trail.cloud.yandex.net"
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
  default     = "25"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "host_group" {
  description = "SVM host group"
  default     = "service"
}

variable "instance_platform_id" {
  description = "instance platform id"
  default     = "standard-v2"
}

variable "instance_pci_topology_id" {
  default = "V2"
}

variable "security_group_ids" {
  default = ["enp9o9k4rdsi0tlursh2"]
}

variable "name_prefix" {
    default = "prod-tool"
    description = "display prefix name of instance"
}

variable "hostname_prefix" {
    default = "tool"
    description = "hostname prefix of instance"
}

variable "secondary_disk_name" {
  default = "log"
}

variable "secondary_disk_size" {
  default = "25"
}

variable "secondary_disk_type" {
  default = "network-ssd"
}
