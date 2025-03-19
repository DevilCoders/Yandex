UpdateServices-Services:
    win_servermanager.installed:
        - recurse: False
        - name: UpdateServices-Services

UpdateServices-WidDB:
    win_servermanager.installed:
        - recurse: False
        - name: UpdateServices-WidDB
        - require:
            - win_servermanager: UpdateServices-Services

UpdateServices-API:
    win_servermanager.installed:
        - recurse: False
        - name: UpdateServices-API
        - require:
            - win_servermanager: UpdateServices-Services


TEMP-permissions:
    cmd.run:
        - shell: powershell
        - name: >
            $acl = Get-Acl -Path 'C:\Windows\Temp';
            $rule = New-Object System.Security.AccessControl.FileSystemAccessRule("NT Authority\Network Service", "FullControl", "ContainerInherit,ObjectInherit", "None", "Allow");
            $acl.SetAccessRule($rule);
            Set-Acl -Path 'C:\Windows\Temp' $acl
        - unless: >
            $acl = Get-Acl -Path 'C:\Windows\Temp';
            $rule = $acl.Access | Where { $_.IdentityReference -eq 'NT Authority\Network Service' };
            exit [int]($null -eq $rule)
        - require:
            - win_servermanager: UpdateServices-Services
        - require_in:
            - cmd: WSUS-initialised

ASPTEMP-permissions:
    cmd.run:
        - shell: powershell
        - name: >
            $acl = Get-Acl -Path 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\Temporary ASP.NET Files';
            $rule = New-Object System.Security.AccessControl.FileSystemAccessRule("NT Authority\Network Service", "FullControl", "ContainerInherit,ObjectInherit", "None", "Allow");
            $acl.SetAccessRule($rule);
            Set-Acl -Path 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\Temporary ASP.NET Files' $acl
        - unless: >
            $acl = Get-Acl -Path 'C:\Windows\Microsoft.NET\Framework\v4.0.30319\Temporary ASP.NET Files';
            $rule = $acl.Access | Where { $_.IdentityReference -eq 'NT Authority\Network Service' };
            exit [int]($null -eq $rule)
        - require:
            - win_servermanager: UpdateServices-Services
        - require_in:
            - cmd: WSUS-initialised

WSUS-initialised:
    cmd.run:
        - shell: cmd
        - name: '"C:\\Program files\\Update Services\\Tools\\WsusUtil.exe" postinstall CONTENT_DIR=D:\\WSUS'
        - unless: dir D:\WSUS\WsusContent
        - require:
          - cmd: disk-d-mounted

allow_wsus_tcp_8530_in:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_wsus_tcp_8530_in"
    - localport: 8530
    - remoteip: any
    - protocol: tcp
    - dir: in
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_wsus_tcp_8531_in:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_wsus_tcp_8531_in"
    - localport: 8531
    - remoteip: any
    - protocol: tcp
    - dir: in
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready

allow_wsus_tcp_443_out:
  mdb_windows.fw_netbound_rule_present:
    - name: "mdb_wsus_tcp_443_out"
    - localport: any
    - remoteport: any
    - remoteip: any
    - protocol: tcp
    - dir: out
    - action: allow
    - require:
        - test: windows-firewall-req
    - require_in:
        - test: windows-firewall-ready


WSUS-Configured:
    cmd.run:
        - shell: powershell
        - name: >
                $ErrorActionPreference = "Stop";
                $wsus = Get-WSUSServer;
                $wsusconfig = $wsus.GetConfiguration();
                Set-WsusServerSynchronization -SyncFromMU|out-null;
                $wsusconfig.AllUpdateLanguagesDssEnabled = $false;
                $wsusconfig.SetEnabledUpdateLanguages('en');
                $wsusconfig.TargetingMode = 'Client';
                $wsusconfig.OobeInitialized = $true;
                $wsusconfig.save();
                $subscription = $wsus.GetSubscription();
                $subscription.StartSynchronizationForCategoryOnly();
                while ($subscription.GetSynchronizationstatus() -notlike 'NotProcessing') {sleep(2)};
                Get-wsusproduct |Set-WsusProduct -Disable;
                Get-wsusproduct |Where-Object {$_.Product.Title -eq 'Windows Server 2019'}|Set-WsusProduct;
                Get-wsusproduct |Where-Object {$_.Product.Title -in 'Microsoft SQL Server 2016','Microsoft SQL Server 2017','Microsoft SQL Server 2019'}|Set-WsusProduct;
                $groups = $wsus.GetComputerTargetGroups();
                Get-WsusClassification | Where-Object {$_.Classification.Title -in ('Critical Updates','Security Updates')} | Set-WsusClassification;
                $subscription = $wsus.GetSubscription();
                $subscription.SynchronizeAutomatically=$True;
                $subscription.SynchronizeAutomaticallyTimeOfDay= (New-TimeSpan -Hours 0);
                $subscription.NumberOfSynchronizationsPerDay=1;
                $subscription.Save();
                $wsusConfig.Save();
                $wsus.GetSubscription().StartSynchronization();
                echo "Done"|out-file "C:\Program Files\Mdb\WSUS_config";
        - unless: cat "C:\Program Files\Mdb\WSUS_config";

{% set target_group = salt.pillar.get('yandex:environment') %}

WSUS-TargetGroup-Present-{{target_group}}:
    cmd.run:
        - shell: powershell
        - name: >
            $wsus = Get-WSUSServer;
            $wsus.CreateComputerTargetGroup("{{ target_group }}");
        - unless: >
            $wsus = Get-WSUSServer;
            $group = "{{ target_group }}";
            1/(($wsus.GetComputerTargetGroups()|where {$_.Name -like $group}|measure-object).Count);
        - require:
           - cmd: WSUS-Configured


approve-updates-script-present:
    file.managed:
        - name: 'C:\Program Files\Mdb\approve_wsus_updates.ps1'
        - source: salt://{{ slspath }}/conf/approve_wsus_updates.ps1
        - require:
            - file: 'C:\Program Files\Mdb'


{% if target_group == 'compute-prod' %}
{% set delay=14 %}
{% else %}
{% set delay=0 %}
{% endif %}

approve-updates-scheduled:
    mdb_windows_tasks.present:
        - name: approve-wsus-updates
        - command: 'powershell.exe'
        - arguments: '-file "C:\Program Files\Mdb\approve_wsus_updates.ps1" -DaysBack {{ delay }} -TargetGroupName {{ target_group }}'
        - schedule_type: 'Daily'
        - start_time: '00:00'
        - days_interval: 1
        - enabled: True
        - multiple_instances: "No New Instance"
        - force_stop: True
        - location: mdb
        - require:
            - file: approve-updates-script-present

