local windows_common =  {
  type: 'yandex',
  name: '{{user `image_family`}}',
  endpoint: '{{user `endpoint`}}',
  //service_account_key_file: '{{user `service_account_key_file`}}',

  //source_image_name: '{{user `source_image_name`}}',
  source_image_family: '{{user `source_image_family`}}',
  source_image_folder_id: '{{user `source_image_folder_id`}}',

  image_name: '{{user `image_family`}}-v{{isotime \"20060102-0304\"}}',
  image_family: '{{user `image_family`}}',
  image_description: '{{user `image_description`}}',

  folder_id: '{{user `folder_id`}}',
  instance_name: '{{build_name}}-v{{isotime \"20060102-0304\"}}',
  metadata: { 'user-data': 'net user Administrator \'{{user `generated_password`}}\'' },
  platform_id: '{{user `platform`}}',
  instance_cores: '2',
  instance_mem_gb: '8',
  disk_size_gb: 50,
  disk_type: 'network-ssd',
  zone: '{{user `zone`}}',
  subnet_id: '{{user `subnet_id`}}',
  use_internal_ip: 'true',
  use_ipv4_nat: true,

  communicator: 'winrm',
  winrm_username: '{{user `username`}}',
  winrm_password: '{{user `generated_password`}}',
  winrm_use_ssl: 'true',
  winrm_insecure: 'true',
  winrm_use_ntlm: 'true',
  winrm_timeout: '60m',
  state_timeout: '30m',
  max_retries: 30
};

local windows_gpu = windows_common {
  instance_cores: '8',
  instance_mem_gb: '96',
  instance_gpus: 1,
};

local windows_vgpu = windows_common {
  instance_cores: '4',
  instance_mem_gb: '12',
  instance_gpus: 1,
};

local windows_base = windows_common {
  target_image_folder_id: '{{user `target_image_folder_id`}}',
};

{
  windows_common: [windows_common],
  windows_gpu: [windows_gpu],
  windows_vgpu: [windows_vgpu],
  windows_base: [windows_base],
}
