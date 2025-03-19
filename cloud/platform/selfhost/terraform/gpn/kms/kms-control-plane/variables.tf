variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-kms/common
  default = "bn3v39j1bo0qgd0d4emb"
}

variable "service_account_id" {
  // yc-kms/common/sa-kms
  default = "d3bic0s14n0mrnn86tmu"
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "ipv6_addresses" {
  // GPN: 2a0d:d6c0:200:200::7:*
  default = ["2a0d:d6c0:200:200::7:151", "2a0d:d6c0:200:200::7:152", "2a0d:d6c0:200:200::7:153"]
}

variable "ipv4_addresses" {
  // GPN: 172.16.0.*
  default = ["172.16.0.151", "172.16.0.152", "172.16.0.153"]
}

variable "subnets" {
  type = "map"

  default = {
    "ru-gpn-spb99" = "e57ope2brmae22de3r3v"
  }
}

variable "lb_address" {
  default = "2a0d:d6c0:200:204::29a"
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
  default     = "4"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "16"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "20"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
