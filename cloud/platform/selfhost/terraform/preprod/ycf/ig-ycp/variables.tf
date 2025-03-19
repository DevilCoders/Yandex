variable name {
  type        = string
  description = "Visible name"
}

variable description {
  type        = string
  description = "Visible description"
}

variable folder_id {
  type        = string
  description = "Folder to deploy IG to"
}

variable service_account_id {
  type        = string
  description = "SA to be used during provisioning"
}

variable subnets {
  type        = map(string)
  description = "Map of subnet_id to zone_id mapping to be used during deployment"
}

variable amount {
  type        = number
  description = "Total amount of active instances"
}

variable abc_service_name {
  type        = string
  description = "ABC service name"
  default     = "ycserverles"
}

variable service {
  type        = string
  description = "Service identification"
}

variable ssh-keys {
  type        = string
  description = "SSH keys to be added to metadata"
}

variable decode_key {
  type        = string
  description = "[optional] Key to decode secrets (if any)"
  default     = ""
}

variable conductor_group {
  type        = string
  description = "[optional] Conductor group to add instances to"
}

variable l3_tg {
  type        = bool
  description = "Whether should create L3 TG"
  default     = false
}

variable l7_tg {
  type        = bool
  description = "Whether should create L7 TG"
  default     = false
}

# Instance-related
variable instance_service_account_id {
  type        = string
  description = "[optional] SA to be used with instance"
  default     = ""
}

variable platform_id {
  type        = string
  description = "[optional] Compute platform ID. Default: 'standard-v2'"
  default     = "standard-v2"
}

variable cores {
  type        = number
  description = "Instance cores amount"
  default     = 2
}

variable core_fraction {
  type        = number
  description = "Instance core fraction, %"
  default     = 100
}

variable memory {
  type        = number
  description = "Instance RAM amount"
  default     = 2
}

variable boot_disk_size {
  type        = number
  description = "Instance boot disk size"
  default     = 16
}

variable boot_disk_image_id {
  type        = string
  description = "Instance boot disk image ID"
}

variable secondary_disk_size {
  type        = number
  description = "Secondary disk size"
  default     = 5
}

variable instance_metadata {
  type        = map(any)
  description = "[optional] Instance metadata to be merged with defaults"
}
