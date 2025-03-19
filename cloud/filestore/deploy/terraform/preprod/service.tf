
module "filestore-server-vla" {
  source        = "./filestore-server"
  zone          = "vla"
  image_version = var.image_version
}

# module "filestore-server-sas" {
#   source        = "./filestore-server"
#   zone          = "sas"
#   image_version = var.image_version
# }

# module "filestore-server-myt" {
#   source        = "./filestore-server"
#   zone          = "myt"
#   image_version = var.image_version
# }
