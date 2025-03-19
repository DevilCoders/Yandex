// Secrets
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
  type        = string
  sensitive   = true
}

variable "service_account_key_file" {
  description = "Yandex Team security OAuth token"
  type        = string
  sensitive   = true
}


// Target
variable "environment" {
  description = "Environment to deploy in"
  type        = string

  validation {
    condition     = var.environment == "preprod" || var.environment == "staging" || var.environment == "prod"
    error_message = "The environment value must be one of: preprod, staging, prod."
  }
}

variable "folder_id" {
  description = "Yandex Cloud Folder ID where resources will be created"
  type        = string
  # TODO: Add validation block
}

variable "service_account_id" {
  description = "ID of the service account authorized for this instance"
  type        = string
  # TODO: Add validation block
}


# Service description 
variable "name" {
  description = "Common part of the name of the resulting service"
  type        = string
}

variable "description" {
  description = "Common part of the description of the resulting service"
  type        = string
}

variable "components" {
  description = "List of components to use in service"
  # type = list(
  #   object(
  #     {
  #       container = any
  #       configs   = any
  #       volumes   = any
  #       envvar    = any
  #     }
  #   )
  # )
}

// Resources
variable "networks" {
  description = "Networks to use in service instances"
}

variable "zones" {
  description = "Yandex Cloud zones to deploy in"
  type        = list(string)

  # TODO: Add validation for zones validity
}

variable "instance_group_size" {
  description = "Resulting number of instances in instance group"
  type        = number
}

variable "resources_per_instance" {
  description = "Resource of each instance in instance group"
  type = object(
    {
      memory = number
      cores  = number
      gpus   = number
    }
  )
}

variable "instance_disk_desc" {
  description = "Instance disk description"
  type = object(
    {
      size = string
      type = string
    }
  )
}

variable "secondary_disk_specs" {
  description = "Secondary disks description"
  default     = []
}

variable "deploy_policy" {
  description = "Deploy policy of the resulting instance group"
  type = object(
    {
      max_unavailable  = number
      max_creating     = number
      max_expansion    = number
      max_deleting     = number
      startup_duration = string
    }
  )
}

# Custom parameters
variable "image_id" {
  description = "Compute image to use"
  default     = null
}

variable "additional_volumes" {
  description = "Map of additional volumes to be mounted in containers"
  default     = {}
}

variable "additional_envvar" {
  description = "Map of additional envvar to be provided to containers"
  default     = {}
}

variable "additional_runcmd" {
  default = []
}

variable "additional_userdata" {
  default = {}
}

// Legacy parameters for compatibility - Eventually should be removed
variable "target_group_name" {
  default = ""
}

variable "target_group_description" {
  default = ""
}

variable "skm_bundle" {
}

variable "nlb_target" {
  type = bool
  default = false
}

variable "abc_service" {
  type = string
}
