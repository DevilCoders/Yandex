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
  default = ["2a0d:d6c0:200:200::7:101", "2a0d:d6c0:200:200::7:102", "2a0d:d6c0:200:200::7:103"]
}

variable "ipv4_addresses" {
  // GPN: 172.16.0.*
  default = ["172.16.0.101", "172.16.0.102", "172.16.0.103"]
}

variable "subnets" {
  type = "map"

  default = {
    "ru-gpn-spb99" = "e57ope2brmae22de3r3v"
  }
}

variable "lb_address" {
  default = "2a0d:d6c0:200:204::14f"
}

variable "lb_v4_address" {
  default = "10.50.61.136"
}

variable "lb_public_api_port" {
  description = "Balancer public API port"
  default     = "443"
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
  default     = "444"
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
  default     = "40"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
