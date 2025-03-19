include "root" {
  path   = find_in_parent_folders("root.hcl")
}

include "yt_creds" {
  path   = find_in_parent_folders("yt_creds.hcl")
}

include "prov" {
  path   = find_in_parent_folders("providers.hcl")
}

include "version" {
  path   = find_in_parent_folders("piper_version.hcl")
  expose = true
}

dependency "logging" {
  config_path = "../infra/logging"
}


inputs = {
  image_name = include.version.locals.image_name
  logging_id = dependency.logging.outputs.piper-logs.id
}
