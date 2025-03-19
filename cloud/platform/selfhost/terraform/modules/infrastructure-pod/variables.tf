variable "hostname" {
  type = string
  default = "default_hostname"
}

variable "metadata_image_version" {
  type = string
  default = "1b931b4a1"
}
variable "juggler_client_image_version" {
  type = string
  default = "2019-03-28T19-19"
}

variable "solomon_agent_image_version" {
  type = string
  default = "2020-01-22T09-25"
}

variable "push-client_image_version" {
  type = string
  default = "2019-03-28T18-40"
}

variable "logcleaner_image_version" {
  type = string
  default = "cc0eb217c"
}

variable "fluent_image_version" {
  type = string
  default = "d847c0cdf"
}
