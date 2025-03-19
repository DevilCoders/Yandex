variable "instance_group_size" {
  default = 3
}

variable "dc_mesh" {
  default = true
}

variable "name_prefix" {
  default     = "prefix"
  description = "Prefix to form instance name"
}

variable "hostname_prefix" {
  default     = "prefix"
  description = "Prefix to form instance hostname"
}

variable "ipv6" {
  default = true
}

variable "ssh-keys" {
  description = "Instance metdata field 'ssh-keys'"
  default     = ""
}

variable "podmanifest" {
  description = "Kubelet pod manifest"
  default     = ""
}

variable "configs" {
  description = "configs files"
  default     = ""
}

variable "infra-configs" {
  description = "infrastructure configs files ( fluent/juggler/solomon/... )"
  default     = ""
}

variable "docker-config" {
  description = "docker auth config"
  default     = ""
}

variable "skip_update_ssh_keys" {
  default = "false"
}

variable "instance_description" {
  default = "default instance description for IG node"
}

variable "role_name" {
  default = "Role name - should be part of hostname"
}

variable "cores_per_instance" {
  default     = 1
  description = "CPU cores per one instance"
}

variable "disk_per_instance" {
  default     = 4
  description = "Boot disk size per one instance"
}

variable "disk_type" {
  default     = "network-hdd"
  description = "One of disk types. Supported listed at https://nda.ya.ru/3VQZR6"
}

variable "memory_per_instance" {
  default     = 1
  description = "Memory in GB per one instance"
}

variable "image_id" {
  description = "Image ID for instance creation"
}

variable "subnets" {
  type        = "map"
  description = "Map of zone -> subnet_id for use"
}

///
variable "zones" {
  type = "list"

  default = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]
}

variable "host_suffix_for_zone" {
  type = "map"

  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

variable "metadata" {
  description = "Additional metadata keys (will be mergbed with module-based)"
  type        = "map"
  default     = {}
}

variable "labels" {
  description = "Additional labels for instance (will be mergbed with module-based)"
  type        = "map"
  default     = {}
}
