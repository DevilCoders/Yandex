variable "image_urls_map" {
  type    = map(string)
  default = {}
}

variable "folder_id" {
  type = string
  description = "Ihe id of the folder for image and placement group"
}
