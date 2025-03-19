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
    default = 1
}

variable "disk_size" {
    default = 20
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

variable "ssh_keys_path" {
    default = ""
}

variable "sec_disk_size" {
    default = 20
}

variable "jaeger-endpoint" {
    default = ""
}

variable "instance_memory" {
    default = 4
}

variable "redeploy" {
    default = 1
}
