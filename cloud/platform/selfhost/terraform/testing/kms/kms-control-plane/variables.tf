variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms/common
  default = "batihg47m3qoolr663qv"
}

variable "service_account_id" {
  // yc-kms/common/sa-kms
  default = "d26cdc942kov29jncbs7"
}

variable "lb_address" {
    default = "2a0d:d6c0:0:ff1f::da"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "2"
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:501:0:f810:0:*
  // SAS: 2a02:6b8:c02:901:0:f806:0:*
  // MYT: 2a02:6b8:c03:501:0:f810:0:*
  default = ["2a02:6b8:c0e:2c0:0:fc1e:0:151", "2a02:6b8:c03:8c0:0:fc1e:0:151"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = ["172.16.0.151", "172.18.0.151"]
}

variable "subnets" {
  type = "map"

  default = {
    "ru-central1-a" = "emati62mlqhr28tu0hlt"
    "ru-central1-b" = "fkpd4jul2gul5btf19dj"
    "ru-central1-c" = "flqcabmnj6oq10avops6"
  }
}

variable "lb_private_api_port" {
  description = "Balancer private API port"
  default     = "8443"
}

variable "private_api_port" {
  description = "Must be the same as grpcServer port in application.yaml"
  default     = "9443"
}

variable "health_check_path" {
  description = "Path used by LB for healthcheck"
  default     = "/"
}

variable "health_check_port" {
  description = "Port used by LB for healthcheck"
  default     = "8444"
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

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
