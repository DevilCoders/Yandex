local builders = import '../jsonnet-lib/packer-windows-builders.libsonnet';
local variables = import '../jsonnet-lib/packer-windows-variables.libsonnet';
local provisioners = import '../jsonnet-lib/packer-windows-provisioners.libsonnet';
local post_processors = import '../jsonnet-lib/packer-common-postprocessors.libsonnet';

{
  'variables': variables.windows_base {
    //source_image_name: 'windows-2019-std-qemu',
    source_image_family: 'windows-2019-std-base',
    image_description: 'Windows Server 2019 Standard Base Image',
    image_family: 'windows-2019-std-base',
    target_image_folder_id: 'b1g8fqssmhljjerknber',
  },
  'builders': builders.windows_base,
  'provisioners': provisioners.base,
  'post-processors': post_processors.fabric_common,
}