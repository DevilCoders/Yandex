variable "image_id" {
    default = ""
}

variable "user_data_path" {
    default = ""
}

variable "installation" {
    default = ""
}

variable "sa_ig" {
    default = ""
}

variable "subnets" {
    default = {}
}

variable "ig_size" {
    default = 0
}

variable "sa_kms" {
    default = ""
}

variable "folder_id" {
    default = ""
}

variable "startup_duration" {
    default = "0s"
}

variable "redeploy" {
    default = 1
}

variable "core_fraction" {
    default = 100
}
