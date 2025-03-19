variable "environment" {
  description = "Environment to deploy in"
  type        = string

  validation {
    condition     = var.environment == "preprod" || var.environment == "staging" || var.environment == "prod"
    error_message = "The environment value must be one of: preprod, staging, prod."
  }
}

variable "tag" {
  default = null
}

variable "solomon_provider" {

}


variable "solomon_cluster" {

}

// Will go to modules
variable "push_sources" {
  type = list(object(
    {
      name         = string
      bind_address = string
      bind_port    = number
      handlers = list(object(
        {
          project  = string
          service  = string
          endpoint = string
        }
      ))
    }
  ))

  default = []
}

// Will go to static config loader
variable "pull_sources" {
  description = "Sources for read data from into solomon agent"

  type = list(object(
    {
      project           = string
      service           = string
      pull_interval     = string
      url               = string
      format            = string
      additional_labels = map(string)
    }
  ))

  default = []
}

// Will go to static config loader
variable "system_sources" {
  type = list(object(
    {
      project           = string
      service           = string
      pull_interval     = string
      level             = string
      additional_labels = map(string)
    }
  ))

  default = []
}

variable "shards" {
  description = "Sources for read data from into solomon agent"

  type = list(object(
    {
      project  = string
      service  = string
      preserve = bool

      override = object(
        {
          project = string
          cluster = string
          service = string
        }
      )
    }
  ))

  default = []
}