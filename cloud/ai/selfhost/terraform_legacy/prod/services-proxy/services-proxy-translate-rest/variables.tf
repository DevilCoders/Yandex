////////
// Auth data
variable "yandex_token" {
  description = "Yandex Team security OAuth token"
}

// Instance group config
variable "max_unavailable" {
  description = "The maximum number of running instances that can be taken offline (stopped or deleted) at the same time during the update process"
  default     = 1
}

variable "max_creating" {
  description = "The maximum number of instances that can be created at the same time"
  default     = 30
}

variable "max_expansion" {
  description = "The maximum number of instances that can be temporarily allocated above the group's target size during the update process"
  default     = 0
}

variable "max_deleting" {
  description = "The maximum number of instances that can be deleted at the same time"
  default     = 5
}
