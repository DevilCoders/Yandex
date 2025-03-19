variable "tvm_secret" {
  type = string
  sensitive = true
  default = ""
}

variable "mongo_users" {
  type = map(string)
  sensitive = true
  default = {
    "gore"    = "",
    "andgein" = "",
  }
}
