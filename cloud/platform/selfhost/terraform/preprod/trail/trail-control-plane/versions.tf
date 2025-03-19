variable "application_version" {
  description = "Version of docker image with trail-control-plane"
  default     = "25225-9c137a6c71"
}

variable "solomon_agent_image_version" {
  type    = "string"
  default = "2021-07-07T15-31"
}
