// General
variable "folder_id" {
  description = "ID of the folder that the resources belong to"
}

variable "service_account_id" {
  description = "ID of the service account authorized for this instance group"
}

// Instance group parameters
variable "name" {
  description = "Name of the instance group"
  default     = "test-ig"
}

variable "description" {
  description = "A description of the instance group"
  default     = ""
}

variable "instance_group_size" {
  description = "Number of compute instances"
  default     = 3
}

variable "zones" {
  type = list(string)

  default = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]
}

variable "target_group_name" {
  description = "Name of the target group"
  default     = "test-ig"
}

variable "target_group_description" {
  description = "A description of the target group"
  default     = ""
}

// Deployment policy
variable "max_unavailable" {
  description = "The maximum number of running instances that can be taken offline (stopped or deleted) at the same time during the update process"
  default     = 2
}
variable "max_creating" {
  description = "The maximum number of instances that can be created at the same time"
  default     = 2
}
variable "max_expansion" {
  description = "The maximum number of instances that can be temporarily allocated above the group's target size during the update process"
  default     = 2
}
variable "max_deleting" {
  description = "The maximum number of instances that can be deleted at the same time"
  default     = 2
}

variable "startup_duration" {
  description = "The amount of time in seconds to allow for an instance to start. Instance will be considered up and running (and start receiving traffic) only after the startup_duration has elapsed and all health checks are passed"
  default     = 60
}

// Instance parameters
variable "instance_description" {
  description = "Default instance description for IG node"
  default     = ""
}

variable "platform_id" {
  description = "The ID of the hardware platform configuration for the instance"
  default     = "standard-v1"
}

variable "cores_per_instance" {
  description = "CPU cores per one instance"
  default     = 1
}

variable "disk_per_instance" {
  description = "Boot disk size per one instance"
  default     = 4
}

variable "gpus_per_instance" {
  description = "GPUs per one instance"
  default     = 0
}

variable "disk_type" {
  description = "One of disk types. Supported listed at https://nda.ya.ru/3VQZR6"
  default     = "network-hdd"
}

variable "memory_per_instance" {
  description = "Memory in GB per one instance"
  default     = 1
}

variable "subnets" {
  description = "Map of zone -> subnet_id for use"
  type        = map(string)
}

variable "ipv6" {
  description = "Use ipv6 interface"
  default     = true
}

variable "image_id" {
  description = "Image ID for instance creation"
}

// Application config
variable "podmanifest" {
  description = "Kubelet pod manifest"
  default     = ""
}

variable "configs" {
  description = "configs files"
  default     = ""
}

variable "docker_config" {
  description = "docker auth config"
  default     = ""
}

variable "skip_update_ssh_keys" {
  default = "false"
}

variable "metadata" {
  description = "Additional metadata keys (will be mergbed with module-based)"
  type        = map(string)
  default     = {}
}

variable "labels" {
  description = "Additional labels for instance (will be mergbed with module-based)"
  type        = map(string)
  default     = {}
}
