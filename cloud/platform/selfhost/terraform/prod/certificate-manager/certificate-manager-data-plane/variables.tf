variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  // yc-cert-manager/default
  default     = "b1g35p5i7e2sm037rgu1"
}

variable "service_account_id" {
  // yc-cert-manager/sa-certificate-manager-dpl
  default = "aje88956l748f2komcjc"
}

variable "security_group_ids" {
  default = ["enp34aatb9dkh959bfdc"]
}

variable "yc_instance_group_size" {
  description = "Number of instances to deploy"
  default     = "3"
}

variable "ipv6_addresses" {
  // VLA: 2a02:6b8:c0e:500:0:f825:3989:*
  // SAS: 2a02:6b8:c02:900:0:f825:3989:*
  // MYT: 2a02:6b8:c03:500:0:f825:3989:*
  // PROD IPs start with 20
  default = ["2a02:6b8:c0e:500:0:f825:3989:21", "2a02:6b8:c02:900:0:f825:3989:21", "2a02:6b8:c03:500:0:f825:3989:21"]
}

variable "ipv4_addresses" {
  // VLA: 172.16.0.*
  // SAS: 172.17.0.*
  // MYT: 172.18.0.*
  // PROD IPs start with 20
  default = ["172.16.0.21", "172.17.0.21", "172.18.0.21"]
}

variable "subnets" {
  type = map(string)

  default = {
    "ru-central1-a" = "e9b43gs1a0m3urpnmv1v"
    "ru-central1-b" = "e2l7lpvob6jqsnf8g530"
    "ru-central1-c" = "b0cvmhc0t3d4auukbji3"
  }
}

variable "subnets_ipv4_addresses" {
  type = map(string)

  default = {
    "e9b43gs1a0m3urpnmv1v" = "172.16.0.21"
    "e2l7lpvob6jqsnf8g530" = "172.17.0.21"
    "b0cvmhc0t3d4auukbji3" = "172.18.0.21"
  }
}

variable "public_ipv4_address" {
  type    = string
  default = "84.201.148.148"
}

variable "public_ipv6_address" {
  type    = string
  default = "2a0d:d6c1:0:1c::3bf"
}

variable "ipv6_address" {
  type    = string
  default = "2a0d:d6c0:0:1c::3d5"
}

variable "subnets_ipv6_addresses" {
  type = map(string)

  default = {
    "e9b43gs1a0m3urpnmv1v" = "2a02:6b8:c0e:500:0:f825:3989:21"
    "e2l7lpvob6jqsnf8g530" = "2a02:6b8:c02:900:0:f825:3989:21"
    "b0cvmhc0t3d4auukbji3" = "2a02:6b8:c03:500:0:f825:3989:21"
  }
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "8"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "32"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "50"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-ssd"
}

variable "private_api_port" {
  description = "Must be the same as grpcServer port in application.yaml"
  default     = "9443"
}

variable "private_api_healthcheck_port" {
  description = "Must be the same as healthcheck port in application.yaml"
  default     = "9444"
}

variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}
