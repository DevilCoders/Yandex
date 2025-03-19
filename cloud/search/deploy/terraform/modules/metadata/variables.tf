variable "env_name" {
  type = string
}

variable "self_dns_api_host" {
  type = string
}

variable "domain" {
  type = string
}

variable "service_name" {
  type = string
}

variable "packet_name" {
  type = string
}

variable "packet_version" {
  type = string
}

variable "lockbox_api_host" {
  type = string
}

variable "selfdns_secret" {
  type = string
}

variable "sa_consumer_secret" {
  type = string
}

variable "sa_consumer_version" {
  type = string
}

variable "searchmap" {
  type = string
}

variable "zk_config" {
  type = map(string)
}

variable "solomon_agent_version" {
  type    = string
  default = "1:17.3"
}
