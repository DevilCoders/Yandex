variable "name" {
  description = "Name of container, should be unique in one podmanifest"
  type        = string
}

variable "environment" {
  description = "Environment to deploy in"
  type        = string

  validation {
    condition     = var.environment == "preprod" || var.environment == "staging" || var.environment == "prod"
    error_message = "The environment value must be one of: preprod, staging, prod."
  }
}

variable "repositories" {
  description = "Map environment -> docker registry with repository path"
  type        = map(string)

  # TODO: Add validation
  # validation {}
}

variable "tag" {
  description = "Tag of Docker container to deploy"
  type        = string

  # TODO: Add validation
  # validation {}
}

variable "ports" {
  description = "List of ports required by container"
  type        = list(number)
  default     = []
}

variable "envvar" {
  description = "List of required variables required by container"
  type        = list(string)
  default     = []
}

variable "mounts" {
  description = "Container mount points"
  type = map(object(
    {
      path      = string
      read_only = bool
    }
  ))
  default = {}
}

variable "command" {
  description = "Adds command section in resulting podmanifest"
  type        = list(string)
  default     = []
}

variable "args" {
  description = "Adds args section in resulting podmanifest"
  type        = list(string)
  default     = []
}

variable "is_init" {
  description = "Flag for adding container in initContainers section"
  type        = bool
  default     = false
}