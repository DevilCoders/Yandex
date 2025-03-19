include "root" {
  path   = find_in_parent_folders("root.hcl")
}

include "yt_creds" {
  path   = find_in_parent_folders("yt_creds.hcl")
}

include "yc_token" {
  path   = find_in_parent_folders("yc_token.hcl")
}
