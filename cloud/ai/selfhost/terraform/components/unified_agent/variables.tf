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

/*
 * TODO: This is late binding solution via referencing by string name
 *       better to implement something like with well_known_networks
 *       when exported from module objects reused as input 
 */
variable "wellknown_logs_routes" {
  default = []
}

variable "custom_logs_routes" {
  default = []
}

variable "wellknown_metrics_routes" {
  default = []
}

variable "custom_metrics_routes" {
  default = []
}

variable "tvm_secret" {
  default = ""
  type = string
}
