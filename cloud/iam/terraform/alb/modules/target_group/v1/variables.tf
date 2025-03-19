variable "name" {
  type = string
}

variable "folder_id" {
  type = string
}

variable "description" {
  type    = string
  default = null
}

variable "target_addresses" {
  type = list(string)
}
