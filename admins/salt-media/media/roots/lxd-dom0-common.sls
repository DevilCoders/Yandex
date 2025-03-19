/etc/systemd/system/systemd-modules-load.service:
  file.managed:
    - contents: |
        [Unit]
        Description=Load Kernel Modules
        Documentation=man:systemd-modules-load.service(8) man:modules-load.d(5)
        DefaultDependencies=no
        Conflicts=shutdown.target
        Before=sysinit.target shutdown.target
        ConditionCapability=CAP_SYS_MODULE
        ConditionDirectoryNotEmpty=|/lib/modules-load.d
        ConditionDirectoryNotEmpty=|/usr/lib/modules-load.d
        ConditionDirectoryNotEmpty=|/usr/local/lib/modules-load.d
        ConditionDirectoryNotEmpty=|/etc/modules-load.d
        ConditionDirectoryNotEmpty=|/run/modules-load.d
        ConditionKernelCommandLine=|modules-load
        ConditionKernelCommandLine=|rd.modules-load

        [Service]
        Type=simple
        Restart=on-abnormal
        RestartSec=5min
        RemainAfterExit=yes
        ExecStart=/lib/systemd/systemd-modules-load
        TimeoutSec=90s
