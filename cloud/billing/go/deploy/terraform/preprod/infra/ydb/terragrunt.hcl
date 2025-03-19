include "root" {
  path   = find_in_parent_folders("root.hcl")
}

include "providers" {
  path   = find_in_parent_folders("providers.hcl")
}
