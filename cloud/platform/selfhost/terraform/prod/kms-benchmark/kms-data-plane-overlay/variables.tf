variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"

  // yc-crypto-e2e-monitoring/benchmark
  default = "b1g2li1qkptm8kfo5g7k"
}

variable "service_account_id" {
  // yc-crypto-e2e-monitoring/benchmark
  default = "ajens7k0jvhhcjlqq5v8"
}

variable "security_group_ids" {
  default = []
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "ipv6_addresses" {
  default = [
    "2a02:6b8:c0e:500:0:f862:0:200",
    "2a02:6b8:c02:900:0:f862:0:200",
    "2a02:6b8:c03:500:0:f862:0:200",
  ]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  default = [
    "172.16.0.100",
    "172.17.0.100",
    "172.18.0.100"
  ]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9btepibuu8dt9h99ark"
    "ru-central1-b" = "e2lj8df713c2linii8kb"
    "ru-central1-c" = "b0cgmc3dcdecc3jns4r0"
  }
}

variable "lb_address" {
  default = "2a0d:d6c0:0:1b::159"
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
  default     = "8444"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "14"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "56"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "25"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

# Must be set via command line params.
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
