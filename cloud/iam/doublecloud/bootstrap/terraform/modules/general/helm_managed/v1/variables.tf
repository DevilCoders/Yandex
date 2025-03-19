variable "name" {
  type = string
}

variable "chart" {
  type = string
}

variable "chart_version" {
  type = string

  default = null
}

variable "env_name" {
  type = string
}

variable "cloud_provider" {
  type = string
}

variable "namespace" {
  type = string

  default = null
}

variable "timeout" {
  type = number

  default = 600
}

variable "values" {
  type = list(string)

  default = []
}

variable "set_values" {
  type = list(object({
    name  = string
    value = string
  }))

  default = []
}

variable "set_sensitive_values" {
  type = list(object({
    name  = string
    value = string
  }))

  default = []
}

variable "kubernetes_provider" {
  type = object({
    cluster_endpoint                   = string
    cluster_certificate_authority_data = string
    api_version                        = string
    kubernetes_provider_command        = string
    kubernetes_provider_args           = list(string)
  })
}
