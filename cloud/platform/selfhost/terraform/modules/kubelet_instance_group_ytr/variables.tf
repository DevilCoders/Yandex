variable "instance_group_size" {
  default = 3
}

variable "name_prefix" {
  default     = "prefix"
  description = "Prefix to form instance name"
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
  type        = list
}

variable "configs" {
  description = "configs files"
  type        = list
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

variable "instance_service_account_id" {
  description = "default service account id"
  default = ""
}

variable "role_name" {
  default = "Role name - should be part of hostname"
}

variable "platform_id" {
  default     = "standard-v1"
  description = "Compute platform ID"
}

variable "cores_per_instance" {
  default     = 1
  description = "CPU cores per one instance"
}

variable "core_fraction_per_instance" {
  default     = 100
  description = "CPU core fraction per one instance"
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
  type        = map
  description = "Map of zone -> subnet_id for use"
}

///
variable "zone_name" {
  type = list

  default = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]
}

variable "zone_suffix" {
  type = list

  default = [
    "vla",
    "sas",
    "myt",
  ]
}

variable "ipv4_addrs" {
  type = list
}

variable "ipv6_addrs" {
  type = list
}

variable "metadata" {
  description = "Additional metadata keys (will be merged with module-based). Added to each instance in the group."
  type        = map
  default     = {}
}

variable "metadata_per_instance" {
  description = "Additional metadata keys (will be merged with module-based). Each entry will be added to the corresponding instance."
  type        = list
  default     = []
}

variable "labels" {
  description = "Additional labels for instance (will be mergbed with module-based)"
  type        = map
  default     = {}
}

variable "secondary_disks" {
  description = "A list of secondary disks. Each entry will be attached to the corresponding instance."
  type = list
  default = []
}
