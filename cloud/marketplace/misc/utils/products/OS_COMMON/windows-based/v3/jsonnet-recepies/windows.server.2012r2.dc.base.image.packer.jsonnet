local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_base {
    //source_image_name: 'windows-2012r2-dc-qemu',
    source_image_family: 'windows-2012r2-dc-base',
    image_description: 'Windows Server 2012R2 Datacenter Base Image',
    image_family: 'windows-2012r2-dc-base',
    target_image_folder_id: 'b1g8fqssmhljjerknber',
  },
  'builders': builders.windows_base,
  'provisioners': provisioners.base,
  'post-processors': post_processors.fabric_common,
}