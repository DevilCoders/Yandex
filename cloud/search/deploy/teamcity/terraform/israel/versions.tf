#variable "solomon_agent_image_version" {
#  type    = "string"
#  default = "2021-07-07T15-31"
#}

variable "backend_docker_version" {
  description = "Version of docker image for queue"
  default     = "0.a19096ec21a4716fe96ff68052ca2df2e791a941.trunk"
}

variable "indexer_docker_version" {
  description = "Version of docker image for indexer"
  default     = "0.d0be8b64dad16885a8ff641ac2161784c4db75cd.trunk"
}

variable "marketplace_backend_docker_version" {
  description = "Version of docker image for mrkt backend"
  default     = "0.a19096ec21a4716fe96ff68052ca2df2e791a941.trunk"
}

variable "proxy_docker_version" {
  description = "Version of docker image for proxy"
  default     = "0.b77e0b4625346a31021d4e8ff0aa7f417ad76f53.trunk"
}

variable "queue_docker_version" {
  description = "Version of docker image for queue"
  default     = "0.fd8106cb60e349edf9c999f0fa2749618666005d.trunk"
}

variable "solomon_version" {
  description = "solomon agent docker images"
  default = "0.be3e70ed44462712820f669a6ff3cea3eb4b4721.trunk"
}

# Logrotate versions
variable "backend_logrotate_docker_version" {
  description = "Version of docker image for backend logrotate"
  default     = "0.a19096ec21a4716fe96ff68052ca2df2e791a941.trunk"
}

variable "indexer_logrotate_docker_version" {
  description = "Version of docker image for indexer logrotate"
  default     = "0.25426a8fbf4bc591b32adffa8984a2802111fd5f.trunk"
}

variable "marketplace_backend_logrotate_docker_version" {
  description = "Version of docker image for marketplace backend logrotate"
  default     = "0.a19096ec21a4716fe96ff68052ca2df2e791a941.trunk" #the same as backend
}

variable "proxy_logrotate_docker_version" {
  description = "Version of docker image for proxy logrotate"
  default     = "0.db6dc066972e6ce5d6d7c0e54f6bc8f7873fd251.trunk"
}

variable "queue_logrotate_docker_version" {
  description = "Version of docker image for queue logrotate"
  default     = "0.4b0578b5f0c820f5649d41c02d2d3842aa14f9f9.trunk"
}
