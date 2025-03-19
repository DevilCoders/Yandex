locals {
  image = format(
    "%s:%s",
    var.repositories[var.environment],
    var.tag
  )

  container = {
    name    = var.name
    image   = local.image
    ports   = var.ports
    envvar  = var.envvar
    mounts  = var.mounts
    command = var.command
    args    = var.args
    is_init = var.is_init
  }
}