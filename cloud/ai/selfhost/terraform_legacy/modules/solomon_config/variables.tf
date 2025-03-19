variable "project" {
    type = string
    default = "cloud_ai"
}

variable "service" {
    type = string
    default = "services_proxy"
}

variable "cluster" {
    type = string
}

variable "shard_service" {
    type = string
}

variable "config_path" {
    type = string
    default = "/etc/solomon-agent"
}

variable "yav_token" {
    type = string
}

variable "yav_id" {
    type = string
}

variable "yav_value_name" {
    type = string
}

variable "sa_public_key_path" {
    type = string
}

variable "sa_private_key_path" {
    type = string
}

variable "loglevel" {
    type = string
    default = "ERROR"
}



