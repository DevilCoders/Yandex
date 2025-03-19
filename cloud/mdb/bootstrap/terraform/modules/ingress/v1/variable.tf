variable "cluster_id" {
  type        = string
  description = "k8s cluster id"
}

variable "folder_id" {
  type        = string
  description = "folder id"
}

variable "subnet_ids" {
  type        = list(string)
  description = "list of subnets where cluster is located"
}
