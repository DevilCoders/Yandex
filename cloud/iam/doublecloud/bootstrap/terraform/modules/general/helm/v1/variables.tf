variable "name" {
  type = string
}

variable "repository" {
  type = string

  default = null
}

variable "chart" {
  type = string
}

variable "chart_version" {
  type = string

  default = null
}

variable "namespace" {
  type = string
}

variable "create_namespace" {
  type = bool

  default = true
}

variable "timeout" {
  type = number

  default = 300
}

variable "values" {
  type = list(string)

  default = null
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
