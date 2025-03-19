variable "fqdn_suffix" {
  type        = string
  default     = "mdb.yandexcloud.net"
  description = "Suffix of primary fqdn"
}

variable "db_name" {
  type        = string
  description = "cluster name"
}

variable "network_id" {
  type        = string
  description = "network id"
}

variable "hosts_placement" {
  type = list(object({
    subnet_id = string
    zone      = string
  }))
  description = "where to place hosts"
}

variable "resources" {
  type = object({
    disk_size          = number
    resource_preset_id = string
    disk_type_id       = string
  })
  description = "resources for database"
}

variable "admin_password" {
  type        = string
  description = "admin user password"
}

variable "connected_security_groups" {
  type        = list(string)
  description = "security groups allowed to connect"
  default     = []
}

variable "database" {
  type = object({
    owner_name     = string
    owner_password = string
    extensions     = list(string)
  })
  description = "map of database names and owners to create"
}

variable "users" {
  type = list(object({
    name       = string
    password   = string
    login      = bool
    conn_limit = number
  }))
  description = "users to create in cluster"
}
