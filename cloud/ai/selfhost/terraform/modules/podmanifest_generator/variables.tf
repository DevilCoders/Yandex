variable "name" {
  description = "Name of the main POD in podmanifest"
  type        = string
}

variable "config_digest" {
  description = "Digest of the rendered config file"
  type        = string
}

variable "containers" {
  type = list(
    object({
      name    = string
      image   = string
      ports   = list(number)
      envvar  = list(string)
      mounts  = map(any)
      command = list(any)
      args    = list(any)
      is_init = bool
    })
  )
}

variable "envvar" {
  type = map(string)
}

variable "volumes" {
  type = map(any)
}