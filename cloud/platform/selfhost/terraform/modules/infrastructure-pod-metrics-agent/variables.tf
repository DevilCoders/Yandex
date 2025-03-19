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
  default = "2019-09-09T15-21"
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
  default = "49062677d"
}
