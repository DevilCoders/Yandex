local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_rdls { licenses_count: '50', },
  'builders': builders.windows_common,
  'provisioners': provisioners.rdls,  
  'post-processors': post_processors.fabric_common,
}