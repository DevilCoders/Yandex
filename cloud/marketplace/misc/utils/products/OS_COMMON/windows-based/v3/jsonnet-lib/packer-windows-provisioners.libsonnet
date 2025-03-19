local shell_local = { type: 'shell-local', };
local noop = shell_local { 
  inline: ['echo \"some time to shutdown :-(\"'], 
  pause_before: '1m',
};
local windows_restart = { type: 'windows-restart', };

local windows_sealed = {
  type: 'powershell',
  inline: [ "& c:\\bootstrap\\windows-scripts\\ensure-sealed.ps1" ],
};

local windows_install = {
  name:: error '"windows_ensure_software" must have name field',

  type: 'powershell',
  elevated_user: 'Administrator',
  elevated_password: '{{user `generated_password`}}',
  environment_vars: [
    'AWS_ACCESS_KEY_ID={{user `mkt_s3_access_key`}}',
    'AWS_SECRET_ACCESS_KEY={{user `mkt_s3_secret_key`}}'
  ],
  inline: '& C:\\bootstrap\\windows-scripts\\invoke-buildstep.ps1 -StepAction Ensure -StepSetting ' + self.name,
};

local windows_configure = {
  name:: error '"windows_setting" must have name field',

  type: 'powershell',
  inline: '& C:\\bootstrap\\windows-scripts\\invoke-buildstep.ps1 -StepAction Ensure -StepSetting ' + self.name,
};

local windows_configure_elevated = {
  name:: error '"windows_setting" must have name field',

  type: 'powershell',
  elevated_user: 'Administrator',
  elevated_password: '{{user `generated_password`}}',
  inline: '& C:\\bootstrap\\windows-scripts\\invoke-buildstep.ps1 -StepAction Ensure -StepSetting ' + self.name,
};

local windows_update = std.flattenArrays([
  std.flattenArrays([[
    windows_restart { restart_timeout: '60m', },
    windows_configure_elevated { name: 'updates'},
  ] for x in std.range(1, 2)])
  + [windows_restart { restart_timeout: '60m', },]
]);

local windows_install_sqlserver = {
  type: 'powershell',
  elevated_user: 'Administrator',
  elevated_password: '{{user `generated_password`}}',
  environment_vars: [
    'AWS_ACCESS_KEY_ID={{user `mkt_s3_access_key`}}',
    'AWS_SECRET_ACCESS_KEY={{user `mkt_s3_secret_key`}}'
  ],
  
  inline: '& C:\\bootstrap\\windows-scripts\\invoke-buildstep.ps1 -StepAction Ensure -StepSetting SQLServer -StepArgs `\"{{user `sql_version`}}`\"'
};

local windows_install_rdls = {
  type: 'powershell',  
  inline: '& C:\\bootstrap\\windows-scripts\\invoke-buildstep.ps1 -StepAction Ensure -StepSetting rdls -StepArgs \'-Count {{user `licenses_count`}} -Agreement {{user `agreement_number`}}\''
};

local windows_bootstrap = {
  type: 'powershell',
  elevated_user: 'Administrator',
  elevated_password: '{{user `generated_password`}}',
  environment_vars: [
    'AWS_ACCESS_KEY_ID={{user `mkt_s3_access_key`}}',
    'AWS_SECRET_ACCESS_KEY={{user `mkt_s3_secret_key`}}'
  ],
  script: "ensure-bootstrapped.ps1",
};

local windows_seal = [
  windows_install { name: 'CloudbaseInit' },
  windows_restart {
    restart_timeout: '30m',
    check_registry: true,  
  },
  windows_sealed,
  noop,
];

// families

local sqlserver = [
  windows_bootstrap,
  windows_install_sqlserver,
  windows_install { name: 'SQLServerManagementStudio' },
] + windows_seal;

local rdls = [
  windows_bootstrap,
  windows_install_rdls,
] + windows_seal;

local gpu = [
  windows_bootstrap,
  windows_install { name: 'nvidiacudapackage' },  
] + windows_seal;

local vgpu = [
  windows_bootstrap,
  windows_install { name: 'NvidiaGRIDDrivers' },
  windows_install { name: 'nvidiagridmodule' },
] + windows_seal;

local ws = [windows_bootstrap] + windows_seal;

local base = [
  windows_bootstrap,
  windows_restart { restart_timeout: '60m', },
  windows_configure {name: 'smb1'},
  windows_configure {name: 'nla'},
  windows_configure {name: 'eth'},
  windows_configure {name: 'icmp'},
  windows_configure {name: 'rdp'},
  windows_configure {name: 'rtu'},
  windows_configure {name: 'serialconsole'},
  windows_configure {name: 'powerplan'},
  windows_configure {name: 'shutdownwithoutlogon'},
] + windows_update + [
  windows_configure {name: 'ngen'},
  windows_configure {name: 'cleanup'},
];

// exported

{
  sqlserver: sqlserver,
  rdls: rdls,
  gpu: gpu,
  vgpu: vgpu,
  ws: ws,
  base: base,
}
