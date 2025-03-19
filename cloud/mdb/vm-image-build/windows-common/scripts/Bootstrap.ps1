New-Item -ItemType directory -Path "C:\Logs" -ErrorAction:SilentlyContinue 

# Log execution
Start-Transcript -IncludeInvocationHeader -Append -NoClobber -Path "C:\Logs\Bootstrap.log"

# Stop at every error
$ErrorActionPreference = "Stop"

$product = (Get-Content 'A:\PRODUCT').Trim()
$product_version = (Get-Content 'A:\PRODUCT_VERSION').Trim()

#set the build parameters

switch ($product) {
    'wsus' {$suite = 'windows'
            $datadisk_required = $false
            }
    'sqlserver' {$suite = 'sqlserver'
                $datadisk_required = $true
                }
    'windows-witness' { $suite = 'windows'
                        $datadisk_required = $false
                      }
}




if (Get-ScheduledTask -TaskName BootstrapAfterSysprep -ErrorAction:SilentlyContinue) {
    Unregister-ScheduledTask -TaskName BootstrapAfterSysprep -ErrorAction:SilentlyContinue -Confirm:$false
}

#################### FUNCTIONS

function SetSaltStartupAccount{
    param(
        [string]$username,
        [string]$password
        )
    Write-Host $(Get-Date) "DEBUG: Change salt minion startup type"
    $service = gwmi win32_service  -filter "name='salt-minion'"
    $service.change($null, $null, $null, $null, $null, $null, "$env:computername\$username", $password)
}

function DownloadAndVerify($Source, $Dest) {
    Start-BitsTransfer -Source $Source -Destination $Dest
    Start-BitsTransfer -Source "${Source}.sha256" -Destination "${Dest}.sha256"
    $hash = Get-FileHash -Path $Dest -Algorithm SHA256 | Select-Object -ExpandProperty Hash
    $hash = $hash.ToUpper()
    $orig = Get-Content -Path "${Dest}.sha256"
    $orig = $orig.Trim().Split()[0].ToUpper()
    if ($hash -ne $orig) {
        throw "$Source sha256 is $hash, while expeced $orig"
    }
}

function RebootAndContinue {
    $TaskTrigger = (New-ScheduledTaskTrigger -AtLogon)
    $TaskAction = New-ScheduledTaskAction -Execute Powershell.exe -argument "-NoProfile -NoLogo -ExecutionPolicy Bypass -File $PSCommandPath"
    Register-ScheduledTask -Force -TaskName BootstrapAfterSysprep -Action $TaskAction -Trigger $TaskTrigger
    Write-Host "Rebooting in 30 seconds..."
    Start-Sleep -Seconds 30
    Restart-Computer -Force
}

function CheckRebootAndContinue {
    # Check Reboot is Required - CBS
    if (Get-ChildItem "HKLM:Software\Microsoft\Windows\CurrentVersion\Component Based Servicing\RebootPending" -ErrorAction Ignore) {
        $RebootRequired = $true
    }
    # Check Reboot is Required - WAU
    if (Get-Item "HKLM:SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate\Auto Update\RebootRequired" -ErrorAction Ignore) {
        $RebootRequired = $true
    }
    # Check Reboot is Required - File Rename Operations
    if (Get-ItemProperty "HKLM:SYSTEM\CurrentControlSet\Control\Session Manager" -Name PendingFileRenameOperations -ErrorAction Ignore) {
        $RebootRequired = $true
    }
    # Reboot and continue
    if ((Get-Command Get-WUIsPendingReboot -ErrorAction:SilentlyContinue) -and (Get-WUIsPendingReboot)) {
        $RebootRequired = $true
    }
    # Reboot and continue
    if ($RebootRequired) {
        RebootAndContinue
    }
}

function Wait-Network {
    try {
        Write-Host "Waiting for network"
        $Retries = 1
        while (-not ($isReachable = Test-Connection 'download.microsoft.com' -Count 1 -Quiet) -and ($Retries -ne 0) ) {
            Start-Sleep -Seconds 10
            $Retries--
        }
    } 
    catch {
        #exit
    }
}


########## MDB: Install virtio drivers
function InstallVirtioDrivers {
    cp A:\* $ENV:TEMP\
    pnputil -a $ENV:TEMP\NETKVM.INF
    pnputil -a $ENV:TEMP\VIOSCSI.INF
    pnputil -a $ENV:TEMP\VIOSTOR.INF
}

########## MDB: Set static IP for buildin
function SetupNetInterface {
    If (-Not (Get-NetIPAddress -IPAddress fd01:ffff:ffff:ffff::2 -ErrorAction:SilentlyContinue)) {
        Disable-NetAdapter -InterfaceDescription "*Virt*" -Confirm:$false
        Rename-NetAdapter -InterfaceDescription "*Intel*" -NewName "eth0" -Confirm:$false
        New-NetIPAddress -InterfaceAlias "eth0" -AddressFamily IPv6 -IPAddress fd01:ffff:ffff:ffff::2 -PrefixLength 96 -DefaultGateway fd01:ffff:ffff:ffff::1
        Set-DnsClientServerAddress -InterfaceAlias "eth0" -ServerAddresses @("2a02:6b8::1:1", "2a02:6b8:0:3400::1")
        Rename-NetAdapter -InterfaceDescription "*Realt*" -NewName "eth1" -Confirm:$false
        Wait-Network
    }
}

########## MDB: Set hostname for template building
function SetupHostname {
    $domainSet = (Get-WmiObject -Class Win32_ComputerSystem).Name -eq "vm-image-template"
    If (-Not $domainSet) {
        Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "Hostname" -Value vm-image-template
        Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "NV Hostname" -Value vm-image-template
        Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "Domain" -Value db.yandex.net
        Set-ItemProperty -Path HKLM:\system\CurrentControlSet\Services\tcpip\parameters -Name "NV Domain" -Value db.yandex.net
    }
}

function PrepareDownloads {
    $exists = Test-Path "$ENV:TEMP\Downloads"
    If (-Not $exists) {
        New-Item -ItemType directory -Path "$ENV:TEMP\Downloads"
    }
}

########## MDB: Install OpenSSH
function InstallOpenSSH {
    $installed = Test-Path "C:\Program Files\OpenSSH-Win64"
    If (-Not $installed) {
        # Download
        Write-Host "Downloading OpenSSH"
        $OpensshURL = 'https://mdb-windows.s3.mds.yandex.net/OpenSSH-Win64.zip'
        $OpensshZipPath = "$ENV:TEMP\Downloads\OpenSSH.zip"

        DownloadAndVerify $OpensshURL $OpensshZipPath
        Expand-Archive $OpensshZipPath -DestinationPath "C:\Program Files"

        # Installing
        Write-Host "Installing OpenSSH"
        & "C:\Program Files\OpenSSH-Win64\install-sshd.ps1"
        New-Item -ItemType directory -Path "C:\ProgramData\ssh" -ErrorAction SilentlyContinue
        Copy-Item "A:\sshd_config" "C:\ProgramData\ssh\sshd_config"
        Start-Service -Name sshd
        Start-Sleep -Seconds 10  # sleep enough for sshd to fill ssh_host_dsa_key


        # Enable firewall
        # TODO: only internal interface ?
        New-NetFirewallRule -Name sshd -DisplayName 'OpenSSH Server (sshd)' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22

        # Set PowerShell as default shell
        New-ItemProperty -Path "HKLM:\SOFTWARE\OpenSSH" -Name DefaultShell -Value "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe" -PropertyType String -Force

        # Autostart SSH
        Set-Service sshd -StartupType Automatic
    }
    else {
        Write-Host "OpenSSH is installed."
    }
}

function CreateTempBuildUser {
    $done = Test-Path "C:\Users\Build\.ssh\authorized_keys"
    If (-Not $done) {
        Write-Host "Creating temp Build user"
        Add-Type -AssemblyName System.Web
        $pass = [System.Web.Security.Membership]::GeneratePassword(64,2)
        $pass = ConvertTo-SecureString $pass -AsPlainText -Force
        New-LocalUser -Name "Build" -Password $pass -Description "User for build script to check errors"
        $cred = New-Object System.Management.Automation.PSCredential -ArgumentList "Build", $pass
        Start-Process cmd /c -Credential $cred -ErrorAction SilentlyContinue -LoadUserProfile
        New-Item -ItemType Directory -Path "C:\Users\Build\.ssh"
        Copy-Item "A:\id_rsa.pub" "C:\Users\Build\.ssh\authorized_keys"
        & "C:\Program Files\OpenSSH-Win64\FixHostFilePermissions.ps1" -Confirm:$false
    }
    else {
        Write-Host "Build user exists"
    }
}

########### MDB: Install Windows Features
function InstallFeatures($Features) {
    Write-Host "Checking if feature installation needed"
    $finstalled = $false
    $Features.split() | Foreach-Object -Process {
        $feature = $_
        $feature_state = Get-WindowsFeature $_
        if (!$feature_state.Installed) {
            Write-Host 'Feature installation required'
            Install-WindowsFeature $feature -IncludeManagementTools
            $finstalled = $true
            Write-Host "Feature $feature installed"
        }
    }
    if ($finstalled) {
    	RebootAndContinue
    }
}

########### MDB: Install Powershell 7
function InstallPoweshell7 {
    $installed = Test-Path "C:\Program Files\PowerShell\7"
    If (-Not $installed) {
        # Download
        Write-Host "Downloading PowerShell"
        $PowershellURL = 'https://mdb-windows.s3.mds.yandex.net/PowerShell-7.0.0-win-x64.msi'
        $PowershellMSIPath = "$ENV:TEMP\Downloads\Powershell.msi"
        DownloadAndVerify $PowershellURL $PowershellMSIPath

        # Installing
        Write-Host "Installing PowerShell"
        Start-Process -FilePath $PowershellMSIPath -ArgumentList "/quiet" -Wait -Passthru

        # Register as Default Shell for OpenSSH
        #New-ItemProperty -Path "HKLM:\SOFTWARE\OpenSSH" -Name DefaultShell -Value "C:\Program Files\PowerShell\7\pwsh.exe" -PropertyType String -Force
        #    New-ItemProperty -Path "HKLM:\SOFTWARE\OpenSSH" -Name DefaultShellCommandOption -Value "-NoLogo -NoProfile" -PropertyType String -Force
    }
    else {
        Write-Host "PowerShell7 is installed."
    }
}

########## MDB: vim
function InstallVim {
    $installed = Test-Path "C:\Program Files (x86)\vim"
    If (-Not $installed) {
        # Download
        Write-Host "Downloading vim"
        $VimURL = 'https://mdb-windows.s3.mds.yandex.net/vim-8.1.55.msi'
        $VimMSI = "$ENV:TEMP\Downloads\vim.msi"
        DownloadAndVerify $VimURL $VimMSI
        # Installing
        Write-Host "Installing vim"
        Start-Process -Wait -FilePath 'msiexec.exe' -ArgumentList "/i $VimMSI /qn"
    }
    else {
        Write-Host "vim is installed."
    }
}

########## MDB: Far Manager
function InstallFar {
    $installed = Test-Path "C:\Program Files\Far Manager"
    If (-Not $installed) {
        # Download
        Write-Host "Downloading Far Manager"
        $FarURL = 'https://mdb-windows.s3.mds.yandex.net/Far30b5757.x64.20210310.msi'
        $FarMSI = "$ENV:TEMP\Downloads\Far.msi"
        DownloadAndVerify $FarURL $FarMSI
        # Installing
        Write-Host "Installing Far Manager"
        Start-Process -Wait -FilePath 'msiexec.exe' -ArgumentList "/i $FarMSI /qn"
        #Add to system Path
        $Environment = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
        $AddPathItems = ";C:\Program Files\Far Manager\;"
        $Environment = $Environment.Insert($Environment.Length,$AddPathItems)
        [System.Environment]::SetEnvironmentVariable("Path", $Environment, "Machine")
    }
    else {
        Write-Host "Far Manager is installed."
    }
}


########## MDB: DBATools

function InstallDbaTools {
    $InstallPath = "C:\windows\system32\WindowsPowerShell\v1.0\Modules\dbatools"
    $installed = (Test-Path $InstallPath)
    if (-not ($installed)){
        Write-host "Downloading DBATools"
        $URL = "https://mdb-windows.s3.mds.yandex.net/dbatools.zip"
        $MSI = "$ENV:TEMP\Downloads\dbatools.zip"
        DownloadAndVerify $URL $MSI
        Write-Host "Installing DBATools"
        mkdir $InstallPath
        Expand-Archive $MSI $InstallPath
        Get-ChildItem -Recurse $InstallPath | Unblock-File 
        Write-Output "DBATools installed"
    }    
}

########## MDB: pyodbc

function InstallPyodbc {
    #Changing ErrorActionPreference temprorarily,
    #to avoid crashing caused by python27 deprecation warning in pip
    $ErrAction = $ErrorActionPreference
    $ErrorActionPreference = 'SilentlyContinue'
    #cd to pip directory
    Set-Location C:\salt\bin\scripts

    #getting list of installed packages
    $packages = &'.\pip2.7.exe' list pyodbc 

    if ($packages -like '*pyodbc*') {
        Write-Host 'PyODBC is already installed'
    }
    else {
        Write-Host 'Installing PyODBC. Trying pip.'
        $installed = &'.\pip2.7.exe' install pyodbc

        if ($installed -like '*Successfully installed pyodbc*') {
            Write-Host 'PyODBC installed from pip'
        }
        else {
            #in case we fail with pip we can fall-back to a copy saved in S3
            InstallPyodbcFromS3
        }
    }
    $ErrorActionPreference = $ErrAction
}
function InstallPyodbcFromS3 {
    $installed = Test-Path "C:\salt\bin\lib\site-packages\pyodbc-*"
    If (-Not $installed) {
        # Download
        Write-Host "Downloading pyodbc"
        $PyODBCURL = 'https://mdb-windows.s3.mds.yandex.net/pyodbc-4.0.30-cp27-cp27m-win_amd64.whl'
        $PyODBCMSI = "$ENV:TEMP\Downloads\pyodbc-4.0.30-cp27-cp27m-win_amd64.whl"
        DownloadAndVerify $PyODBCURL $PyODBCMSI
        $fileexists = test-path $PyODBCMSI
        Write-Host "File Downloaded?: $fileexists"
        # Installing
        Write-Host "Installing pyodbc"
        #saving current location
        $p1 = Get-Location
        #cd to pip directory
        Set-Location C:\salt\bin\scripts
        &'C:\salt\bin\scripts\pip.exe' install $PyODBCMSI
        Write-Host "Finished installing pyodbc"
        Set-Location $p1
        if (Test-Path "C:\salt\bin\lib\site-packages\pyodbc-*") {
            Write-Host "Module found in proper directory"
            Write-Host "Restarting salt-minion"
            Restart-service salt-minion
        }
        else {
            Write-Host "However, module directory couldn't be found where expected...Something went wrong..."
        }
    }
    else {
        Write-Host "pyodbc is installed."
    }
}


function UpdatePipFromS3 {
    #saving current location
    $p1 = Get-Location
    #cd to pip directory
    Set-Location C:\salt\bin\scripts
    $installed = ($(&C:\salt\bin\scripts\pip.exe --version).split(' ')[1] -eq '20.3.4')
    If (-Not $installed) {
        # Download
        Write-Host "Downloading pip"
        $pipURL = 'https://mdb-windows.s3.mds.yandex.net/pip-20.3.4-py2.py3-none-any.whl'
        $pipMSI = "$ENV:TEMP\Downloads\pip-20.3.4-py2.py3-none-any.whl"
        DownloadAndVerify $pipURL $pipMSI
        $fileexists = test-path $pipMSI
        Write-Host "File Downloaded?: $fileexists"
        # Installing
        Write-Host "Installing pip"
        &'C:\salt\bin\scripts\pip.exe' install $pipMSI
        Write-Host "Finished installing pip"
        Set-Location $p1
        Write-Host "Restarting salt-minion"
        Restart-service salt-minion
    }
    else {
        Write-Host "pip is installed."
    }
}


########## MDB: Install Visual C++ Runtime
function InstallVisualCRuntime {
    $software = "*Visual C++*Minimum Runtime*";
    $installed = $null -ne (Get-ItemProperty HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\* | Where-Object { $_.DisplayName -like $software })

    If (-Not $installed) {
        Write-Host "'$software' NOT is installed.";
        # Download
        Write-Host "Downloading vc_redist"
        $VcRedistUrl = 'https://mdb-windows.s3.mds.yandex.net/vc_redist.x64.exe'
        $VcRedistPath = "$ENV:TEMP\Downloads\vc_redist.x64.exe"

        DownloadAndVerify $VcRedistUrl $VcRedistPath

        # Installing
        Write-Host "Installing vc_redist"
        & $VcRedistPath /install /quiet
    }
    else {
        Write-Host "'$software' is installed."
    }
}

########## MDB: Mdb Salt Config
function InstallMdbConfigSalt {
    # TODO: detect last version
    $ConfigSaltUrl = "https://mdb-windows.s3.mds.yandex.net/nupkg/flatcontainer/mdbconfigsalt/1.6394766.0/mdbconfigsalt.1.6394766.0.nupkg"
    $ConfigSaltZipPath = "$ENV:TEMP\Downloads\configsalt.zip"
    $ConfigSaltPath = "C:\Program Files\MdbConfigSalt"
    $installed = Test-Path $ConfigSaltPath
    If (-Not $installed) {
        Write-Host "Installing MdbConfigSalt"
        Start-BitsTransfer -Source $ConfigSaltUrl -Destination $ConfigSaltZipPath
        Expand-Archive $ConfigSaltZipPath -DestinationPath $ConfigSaltPath 
    }
    else {
        Write-Host "MdbConfigSalt is installed"
    }
}

########## MDB: Install Salt Minion
function InstallSaltMinion {
    $installed = Test-Path "C:\salt"
    If (-Not $installed) {
        # Download
        Write-Host "Downloading Salt Minion"
        $SaltMinionUrl = 'https://mdb-windows.s3.mds.yandex.net/Salt-Minion-3000.9-Py2-AMD64-Setup.exe'
        $SaltMinionPath = "$ENV:TEMP\Downloads\Salt-Minion-3000.9-Py2-AMD64-Setup.exe"

        DownloadAndVerify $SaltMinionUrl $SaltMinionPath

        # Installing
        Write-Host "Installing Salt Minion"
        Start-Process -Wait -FilePath $SaltMinionPath -ArgumentList '/S /custom-config="A:\minion_root_config" /start-minion=0'

        # Copy keys
        New-Item -Path "C:\salt\conf\pki\minion" -ItemType Directory -ErrorAction:SilentlyContinue
        Copy-Item "A:\MINION.PUB" "C:\salt\conf\pki\minion\minion.pub"
        Copy-Item "A:\MINION.PEM" "C:\salt\conf\pki\minion\minion.pem"

        # Use deploy v2
        Set-Content -Path "C:\salt\conf\deploy_version" -Value "2"
        Set-Content -Path "C:\salt\conf\mdb_deploy_api_host" -Value "deploy-api.db.yandex-team.ru"

        # set temp hostname
        Set-Content -Path "C:\salt\conf\hostname" -Value "vm-image-template.db.yandex.net"
        Set-Content -Path "C:\salt\conf\minion_id" -Value "vm-image-template.db.yandex.net"

        # Start minion
        Start-Service -Name salt-minion
    }
    else {
        Write-Host "Salt Minion is installed."
    }
}

########## MDB: apply highstate
function ApplyHighstate {
    param (
        $suite
    )
    $applied = Test-Path "$ENV:TEMP/hs_applied" 
    if (-Not $applied) { 
        Write-Host "Applying Highstate"
        $minionconf = "C:\salt\conf\minion"
        Add-Content -Path $minionconf -Value "grains: {test: {suite: $suite, vtype: compute}}"
        $res = Start-Process -PassThru -Wait -NoNewWindow -FilePath C:\salt\salt-call.bat -ArgumentList 'saltutil.sync_all --retcode-passthrough'
        if ($res.Exitcode -ne 0) {
            throw "saltutil.sync_all failed"
        }
        $res = Start-Process -PassThru -Wait -NoNewWindow -FilePath C:\salt\salt-call.bat -ArgumentList 'state.highstate -l debug queue=True --retcode-passthrough' -RedirectStandardError "C:\Logs\Highstate.err.log" -RedirectStandardOutput "C:\Logs\Highstate.out.log"
        if ($res.Exitcode -ne 0) {
            throw "state.highstate failed"
        }
        Set-Content -Path $minionconf -Value (Get-Content -Path $minionconf | Select-String -Pattern '^grains:' -NotMatch)
        New-Item -Path "$ENV:TEMP/hs_applied" -ItemType File
    }
    else {
        Write-Host "Highstate was applied"
    }
}

function PrepareDataDisk {
    Write-Host "Prepare data disk"
    $volume = Get-WmiObject -Class Win32_Volume | Where-Object { $_.DriveLetter -eq "D:" }
    if ($volume.Label -ne "DATA") {
        $volume.DriveLetter = "X:"
        $volume.put()
        Get-Disk | Where-Object { $_.IsBoot -eq $false } | New-Volume -FileSystem NTFS -DriveLetter "D" -FriendlyName "DATA"
    }
}

function InstallSetupScripts {
    Write-Host 'Installing setup scripts'
    New-Item -ItemType directory -Path "C:\Program Files\Mdb" -ErrorAction:SilentlyContinue
    Copy-Item "A:\SetupComplete.ps1" "C:\Program Files\Mdb\SetupComplete.ps1" -ErrorAction:SilentlyContinue
    Set-Content -Path "C:\Program Files\Mdb\reboot.bat" -Value 'shutdown /r /t 0 /d p:2:4'
    # TODO: remove after integration tests are done
    Copy-Item "A:\SetupForTestRun.ps1" "C:\Program Files\Mdb\SetupForTestRun.ps1" -ErrorAction:SilentlyContinue
    # /TODO
    New-Item -ItemType directory -Path "C:\Windows\Setup\Scripts" -ErrorAction:SilentlyContinue 
    Set-Content -Path "C:\Windows\Setup\Scripts\SetupComplete.cmd" -Value 'powershell.exe -NoProfile -NoLogo -WindowStyle hidden -ExecutionPolicy Bypass -File "C:\Program Files\Mdb\SetupComplete.ps1"
    CALL "C:\Program Files\Mdb\reboot.bat"
    '
    Write-Host 'Done installing setup scripts'
}

########## Powerplan 'high performance'
function SetPowerPlan {
    if (-not (powercfg -getactivescheme).contains("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c")) {
        Write-Host "Setting Powerplan"
        powercfg -setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c
    }
}

########## KMS Activation
function KMSActivation {
    # Get Correct Key
    switch ((Get-ItemProperty -Path 'HKLM:\Software\Microsoft\Windows NT\CurrentVersion' -Name ProductName).ProductName) {
        'Windows Server 2012 R2 Datacenter' {
            $KMSClientkey = 'W3GGN-FT8W3-Y4M27-J84CP-Q3VJ9'
        }
        'Windows Server 2016 Datacenter' {
            $KMSClientkey = 'CB7KF-BWN84-R7R2Y-793K2-8XDDG'
        }
        'Windows Server 2019 Datacenter' {
            $KMSClientkey = 'WMDGN-G9PQG-XVVXX-R3X43-63DFG'
        }
    }

    # Set KMS Client Key
    & cscript //nologo $ENV:windir\system32\slmgr.vbs /ipk $KMSClientkey
    Start-Sleep -Seconds 5
    # Set KMS Server
    & cscript //nologo $ENV:windir\system32\slmgr.vbs /skms "kms.cloud.yandex.net"
    Start-Sleep -Seconds 5

    $Retries = 1
    do {
        $Retries--
        # Activate
        & cscript //nologo $ENV:windir\system32\slmgr.vbs /ato
        Start-Sleep -Seconds 5
        $isLicensed = (& cscript //nologo C:\Windows\system32\slmgr.vbs /dli | Select-String -Pattern '^License Status:') -match 'Licensed'
    }
    while (-not $isLicensed -and ($Retries -ge 0))
}

######### Install Windows Updates
function InstallWindowsUpdates {
    if (-not [System.Environment]::GetEnvironmentVariable("BootStrapAutounattend-Updated", "Machine")) {
        Write-Host "Installing Windows Updates"
        $UpdateSession = New-Object -ComObject "Microsoft.Update.Session"
        $UpdateSearcher = $UpdateSession.CreateUpdateSearcher()

        # Windows Updates (System only)
        $UpdateSearcher.ServerSelection = 2

        # Wait Network
        Wait-Network

        # Invoke Search
        Write-Host "Searching updates"
        $SearchResult = $UpdateSearcher.Search("Type='Software' and IsInstalled = 0 and IsHidden = 0").Updates
        Write-Host "Found $($SearchResult.Count) not filtered yet updates"

        # Filter
        Write-Host "Filtering updates"
        $UpdatesToDownload = New-Object -ComObject "Microsoft.Update.UpdateColl"
        foreach ($Update in $SearchResult) {
            if ($Update.EulaAccepted -ne $true) {
                $Update.AcceptEula()
            }
            if (($Update.Title -notlike "*Preview*") -and ($Update.IsBeta -ne $true)) {
                $UpdatesToDownload.Add($Update) | Out-Null
            }
        }

        Write-Host "Found $($UpdatesToDownload.Count) appliable updates"

        if ($UpdatesToDownload.Count -gt 0) {
            Write-Host "Downloading updates"
            #Download updates
            $UpdateDownloader = $UpdateSession.CreateUpdateDownloader()
            $UpdateDownloader.Updates = $UpdatesToDownload
            $UpdateDownloader.Download()

            Write-Host "Installing updates"
            #Install updates
            $UpdateInstaller = New-Object -ComObject Microsoft.Update.Installer
            $UpdateInstaller.Updates = $UpdatesToDownload
            $UpdateInstaller.Install()

            CheckRebootAndContinue
        }
        else {
            # no rerun
            [System.Environment]::SetEnvironmentVariable("BootStrapAutounattend-Updated", $true, "Machine")
        }
    }
}

# UpdatePackageManagement to support v3 protocol
function UpdatePackageManagement {
    $PackageManagementUrl = 'https://mdb-windows.s3.mds.yandex.net/packagemanagement.1.4.6.nupkg'
    $PackageManagementZipPath = "$ENV:TEMP\Downloads\pm.zip"
    $PackageManagementModulePath = "C:\Program Files\WindowsPowerShell\Modules\PackageManagement\1.4.6"
    $installed = Test-Path $PackageManagementModulePath
    if (-Not $installed) {
        Write-Host "Updating package management module"
        # nupkg is just a zip
        DownloadAndVerify $PackageManagementUrl $PackageManagementZipPath 
        Expand-Archive $PackageManagementZipPath -DestinationPath $PackageManagementModulePath
        Remove-Module PackageManagement -ErrorAction:SilentlyContinue
        Import-Module PackageManagement -Force
    }
    else {
        Write-Host "package management module already updated"
    }
}

# InstallYAMLModule
function InstallYAMLModule {
    $PSYAMLUrl = 'https://mdb-windows.s3.mds.yandex.net/powershell-yaml.0.4.1.nupkg'
    $PSYAMLZipPath = "$ENV:TEMP\Downloads\ps-yaml.zip"
    $PSYAMLModulePath = "C:\Program Files\WindowsPowerShell\Modules\powershell-yaml\0.4.1"
    $installed = Test-Path $PSYAMLModulePath
    if (-Not $installed) {
        Write-Host "Installing YAML module"
        # nupkg is just a zip
        DownloadAndVerify $PSYAMLUrl $PSYAMLZipPath 
        Expand-Archive $PSYAMLZipPath -DestinationPath $PSYAMLModulePath
        Import-Module powershell-yaml -Force
    }
    else {
        Write-Host "YAML module already installed"
    }
}

######### EnableSerialConsole
function EnableSerialConsole {
    bcdedit /ems "{current}" on
    bcdedit /emssettings EMSPORT:2 EMSBAUDRATE:115200
    Write-Host "Serial console enabled"
}

######### Cleanup
function Cleanup {
    if (-not [System.Environment]::GetEnvironmentVariable("BootStrapAutounattend-Cleaned", "Machine")) {
        # Remove temp user
        Write-Host 'Cleaning up'
        Remove-LocalUser -Name Build
        Remove-Item -Path "C:\Users\Build\.ssh" -ErrorAction:SilentlyContinue -Recurse -Force
        Remove-Item -Path "C:\Users\Build" -ErrorAction:SilentlyContinue -Recurse -Force

        # Disable minion auto-start
        Set-Service -StartupType Disabled salt-minion

        # Clean hostname files
        Remove-Item -Path "C:\salt\conf\hostname"
        Remove-Item -Path "C:\salt\conf\minion_id"

        # Removing downloaded updates
        Stop-Service wuauserv
        Remove-Item "$ENV:windir\SoftwareDistribution\*" -ErrorAction:SilentlyContinue -Recurse
        Start-Service wuauserv

        # Removing files
        Remove-Item -Path "$ENV:windir\PANTHER\unattend.xml"
        Remove-Item -Path "$ENV:windir\PANTHER\miglog.xml" -ErrorAction:SilentlyContinue
        Remove-Item -Path "$ENV:windir\PANTHER\" -Filter '*.log' -ErrorAction:SilentlyContinue -Recurse
        Remove-Item -Path "$ENV:windir\TEMP\" -ErrorAction:SilentlyContinue -Recurse
        Remove-Item -Path "$env:windir\Prefetch\" -ErrorAction:SilentlyContinue -Recurse
        Remove-Item -Path "$ENV:TEMP\Downloads\" -Recurse
        Remove-Item -Path "$ENV:TEMP\TEMP\" -ErrorAction:SilentlyContinue -Recurse
        Remove-Item -Path "$ENV:TMP\TEMP\" -ErrorAction:SilentlyContinue -Recurse
        Remove-Item -Path "$ENV:ProgramData\Microsoft\Windows\WER\ReportArchive" -ErrorAction:SilentlyContinue -Recurse
        Remove-Item -Path "$ENV:ProgramData\Microsoft\Windows\WER\ReportQueue" -ErrorAction:SilentlyContinue -Recurse
        Clear-RecycleBin -Force -Confirm:$false

        # Running complex clean, thou there can be no cleanmgr
        if (Test-Path C:\Windows\System32\cleanmgr.exe) {
            foreach ($Key in (Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\" | Select-Object -ExpandProperty Name)) {
                New-ItemProperty `
                    -Path "$Key" `
                    -Name "StateFlags1234" `
                    -Value 2 `
                    -PropertyType DWord `
                    -ErrorAction:SilentlyContinue
            }

            #Start-Process -FilePath "cleanmgr.exe" -ArgumentList "/sagerun:1234" -Wait -ErrorAction:SilentlyContinue

            foreach ($Key in (Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\" | Select-Object -ExpandProperty Name)) {
                Remove-ItemProperty `
                    -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\$Key" `
                    -Name StateFlags1234 -ErrorAction:SilentlyContinue `
            
            }
        }

        # Clean Image
        #Start-Process -FilePath 'dism.exe' -ArgumentList "/Online /Cleanup-Image /StartComponentCleanup" -Wait
        Start-Process -FilePath 'dism.exe' -ArgumentList "/Online /Cleanup-Image /StartComponentCleanup /ResetBase" -Wait
        #Start-Process -FilePath 'dism.exe' -ArgumentList "/Online /Cleanup-Image /SPSuperseded" -Wait

        # Clean Recoveries
        Start-Process -FilePath 'vssadmin.exe' -ArgumentList "delete shadows /all /quiet" -Wait

        # Clean Eventlog
        wevtutil el |  Foreach-Object { wevtutil cl "$_" }

        # Optimize Disk
        Optimize-Volume -DriveLetter C -Defrag

        # Disable Defrag
        Get-ScheduledTask -TaskName 'ScheduledDefrag' | Disable-ScheduledTask

        # Check Reboot
        Write-Host "Checking if reboot needed"

        # No rerun
        [System.Environment]::SetEnvironmentVariable("BootStrapAutounattend-Cleaned", $true, "Machine")

    }
}

########## Harden firewall rules
#hardening firewall rules
function HardenWindows {
    Write-Host 'Configuring firewall policy'
    $groups_in = @("Cortana",
        "Work or school account",
        "Cast to Device functionality",
        "DIAL protocol server",
        "mDNS",
        "AllJoyn Router",
        "Your account",
        "File and Printer Sharing"
    )

    Get-NetFirewallRule | Where-Object { $_.Direction -like "Inbound" -and $_.Enabled -eq $True -and $_.DisplayGroup -in $groups_in } | disable-NetFirewallRule
    Disable-NetFirewallRule -DisplayName "Windows Remote Management (HTTP-In)"

    $groups_out = @("AllJoyn Router",
        "Cast to Device functionality"
        "Cortana",
        "DiagTrack",
        "Email and accounts",
        "SmartScreen",
        "Windows Default Lock Screen",
        "Windows Shell Experience",
        "Work or school account",
        "Xbox Game UI",
        "Your account"
    )

    Get-NetFirewallRule | Where-Object { $_.Direction -like "Outbound" -and $_.Enabled -eq $True -and $_.DisplayGroup -in $groups_out } | disable-NetFirewallRule

    #Disabling unnecessary services
    $services = @("DiagTrack", "lfsvc", "PcaSvc", "RmSvc", "wlidsvc", "WpnService")
    Get-service | Where-Object { $_.Name -in $services -and $_.Status -eq "Running" } | set-service -StartupType Disabled
    Get-service | Where-Object { $_.Name -in $services -and $_.Status -eq "Running" } | Stop-Service

     #Firewall_Config
    New-NetFirewallRule -DisplayName "mdb_icmpv6_all_in" -Direction Inbound -Protocol ICMPv6 -IcmpType any -Action Allow
    New-NetFirewallRule -DisplayName "mdb_icmpv4_all_in" -Direction Inbound -Protocol ICMPv4 -IcmpType any -Action Allow
    New-NetFirewallRule -DisplayName "mdb_icmpv4_all_out" -Direction Outbound -Protocol ICMPv4 -IcmpType any -Action Allow
    New-NetFirewallRule -DisplayName "mdb_icmpv6_all_out" -Direction Outbound -Protocol ICMPv6 -IcmpType any -Action Allow


    
    #NTP
    New-NetFirewallRule -DisplayName "mdb_udp_123_out" -Direction Outbound -Protocol UDP -RemotePort 123 -Action Allow
   
    #salt
    netsh advfirewall firewall add rule name= "mdb_salt_out" dir=out action=allow protocol=TCP remoteport=4505-4506 remoteip=any
    #dns
    netsh advfirewall firewall add rule name= "mdb_dns_udp_out" dir=out action=allow protocol=UDP remoteport=53 remoteip=any
    netsh advfirewall firewall add rule name= "mdb_dns_tcp_out" dir=out action=allow protocol=TCP remoteport=53 remoteip=any
   
    #dhcp
    netsh advfirewall firewall add rule name= "mdb_dhcp6_out" dir=out action=allow protocol=UDP remoteport=547 remoteip=any
    netsh advfirewall firewall add rule name= "mdb_dhcp_out" dir=out action=allow protocol=UDP remoteport=67 remoteip=any
    netsh advfirewall firewall add rule name= "mdb_dhcp_in" dir=in action=allow protocol=UDP localport=68 remoteip=any
    #remove all the built-in standard rules
    Get-NetFirewallRule | Where-Object { $_.DisplayName -notlike 'mdb_*' -and $_.DisplayName -notlike "OpenSSH Server (sshd)" -and $_.Enabled -eq 'true' } | disable-NetFirewallRule

}

########## Configure WinRM and RDP
function EnableWinRM {
    #Enable RDP (comment if not needed)
    #Set-ItemProperty -Path "HKLM:\System\CurrentControlSet\Control\Terminal Server" -Name "fDenyTSConnections" -Value 0
    #Enable-NetFirewallRule -DisplayGroup "Remote Desktop"

    
    # Set Real Time
    Set-ItemProperty -Path 'HKLM:\SYSTEM\CurrentControlSet\Control\TimeZoneInformation' -name "RealTimeIsUniversal" -Value 1 -Type DWord -force

    # Enable EMS SAC serial console
    #& bcdedit /ems "{current}" on
    #& bcdedit /emssettings EMSPORT:2 EMSBAUDRATE:115200


    #Write-Host "Configuring WinRM"
    # WinRM will be enabled when there will be security groups in network

    #"communicator": "winrm",
    #      "winrm_username": "{{user `username`}}",
    #      "winrm_password": "{{user `password`}}",
    #      "winrm_timeout": "300m",
    #      "winrm_insecure": "true", # Certificate is self-signed
    #      "winrm_use_ssl": "true",
    #      "winrm_use_ntlm": "true",

    # Disable Unencrypted WinRM Firewall Rule
    Disable-NetFirewallRule -DisplayName "Windows Remote Management (HTTP-In)"

    # Set Connection Profile to Private
    #Start-Sleep -Seconds 60
    #Set-NetConnectionProfile -NetworkCategory Public

    # Remove Listners
    #Remove-Item -Path WSMan:\Localhost\listener\listener* -Recurse

    # Create Certificate
    #$SelfSignedCert = New-SelfSignedCertificate `
    #    -DnsName $Env:COMPUTERNAME `
    #    -CertStoreLocation Cert:\LocalMachine\My

    # Create HTTPS Listner
    #New-Item -Path WSMan:\LocalHost\Listener `
    #    -Transport HTTPS `
    #    -Address * `
    #    -CertificateThumbPrint $SelfSignedCert.Thumbprint `
    #    -Force

    # Could be commented
    #Restart-Service WinRM
    #Start-Sleep -Seconds 60

    # Create Firewall Profile Rule
    New-NetFirewallRule -DisplayName "Windows Remote Management (HTTPS-In)" `
        -Name "Windows Remote Management (HTTPS-In)" `
        -Profile Any `
        -LocalPort 5986 `
        -Protocol TCP

    # Keep it disabled before security groups
    Disable-NetFirewallRule -DisplayName "Windows Remote Management (HTTPS-In)"
}

function GrantRunAsAService {
    $username = 'Administrator'
    Invoke-Command -Script {
        param([string] $username)
        $tempPath = [System.IO.Path]::GetTempPath()
        $import = Join-Path -Path $tempPath -ChildPath "import.inf"
        if (Test-Path $import) { Remove-Item -Path $import -Force }
        $export = Join-Path -Path $tempPath -ChildPath "export.inf"
        if (Test-Path $export) { Remove-Item -Path $export -Force }
        $secedt = Join-Path -Path $tempPath -ChildPath "secedt.sdb"
        if (Test-Path $secedt) { Remove-Item -Path $secedt -Force }
        try {
            Write-Host ("Granting SeServiceLogonRight to user account: {0} on host: {1}." -f $username, $computerName)
            $sid = ((New-Object System.Security.Principal.NTAccount($username)).Translate([System.Security.Principal.SecurityIdentifier])).Value
            secedit /export /cfg $export
            $sids = (Select-String $export -Pattern "SeServiceLogonRight").Line
            foreach ($line in @("[Unicode]", "Unicode=yes", "[System Access]", "[Event Audit]", "[Registry Values]", "[Version]", "signature=`"`$CHICAGO$`"", "Revision=1", "[Profile Description]", "Description=GrantLogOnAsAService security template", "[Privilege Rights]", "$sids,*$sid")) {
                Add-Content $import $line
            }
            secedit /import /db $secedt /cfg $import
            secedit /configure /db $secedt
            gpupdate /force
            Remove-Item -Path $import -Force
            Remove-Item -Path $export -Force
            Remove-Item -Path $secedt -Force
        }
        catch {
            Write-Host ("Failed to grant SeServiceLogonRight to user account: {0} on host: {1}." -f $username, $computerName)
            $error[0]
        }
    } -ArgumentList $username
}


function ConfigureTimeService {
    Write-Host 'Configuring time service'
    net stop w32time
    w32tm /config /syncfromflags:manual /manualpeerlist:"ntp1.yandex.net ntp2.yandex.net ntp3.yandex.net ntp4.yandex.net"
    w32tm /config /reliable:yes
    net start w32time
    bcdedit /set USEPLATFORMCLOCK on
}


function UpdatePassword {
    Write-Host 'Updating password'
    Add-Type -AssemblyName System.Web
    $pass = [System.Web.Security.Membership]::GeneratePassword(64,2)
    [xml] $xml = Get-Content A:\SysprepUnattend.xml
    $xml.unattend.settings[1].component.UserAccounts.AdministratorPassword.Value = $pass
	$xml.Save('A:\SysprepUnattend.xml')
}

function InstallKB4577668 {
    $update = 'kb4577668';
    $installed = $null -ne (Get-Hotfix | Where-Object { $_.hotfixid -like $update -and $_.InstalledOn })

    If (-Not $installed) {
        Write-Host "'$update' is NOT installed.";
        # Download
        Write-Host "Downloading $update"
        $updateUrl = "https://mdb-windows.s3.mds.yandex.net/$update.msu"
        $updatePath = "$ENV:TEMP\Downloads\$update.msu"

        DownloadAndVerify $updateUrl $updatePath

        # Installing
        Write-Host "Installing $update"
        if (-Not (Test-Path C:\Users\Administrator\AppData\Local\Temp\Downloads\kb4577668)) {
            mkdir C:\Users\Administrator\AppData\Local\Temp\Downloads\kb4577668
        }
        expand -f:* "C:\Users\Administrator\AppData\Local\Temp\Downloads\kb4577668.msu" C:\Users\Administrator\AppData\Local\Temp\Downloads\kb4577668
        DISM.exe /Online /Add-Package /PackagePath:C:\Users\Administrator\AppData\Local\Temp\Downloads\kb4577668\Windows10.0-KB4577668-x64_PSFX.cab /norestart
        
        RebootAndContinue 
    }
    else {
        Write-Host "$update is installed."
    }
}

function UpgradeEdition {
    $target_edition = 'ServerDatacenterCor'
    $edition = (Get-Windowsedition -Online).Edition
    if (-Not ($edition -eq $target_edition)) {
        Write-Output "Changing OS Edition from $edition $target_edition"
        $product_name = (Get-ItemProperty -Path 'HKLM:\Software\Microsoft\Windows NT\CurrentVersion' -Name ProductName).ProductName
        switch ($product_name) {
            'Windows Server 2012 R2 Datacenter' {
                $KMSClientkey = 'W3GGN-FT8W3-Y4M27-J84CP-Q3VJ9'
            }
            'Windows Server 2016 Datacenter' {
                $KMSClientkey = 'CB7KF-BWN84-R7R2Y-793K2-8XDDG'
            }
            'Windows Server 2019 Datacenter' {
                $KMSClientkey = 'WMDGN-G9PQG-XVVXX-R3X43-63DFG'
            }
            'Windows Server 2019 Datacenter Evaluation' {
                $KMSClientkey = 'WMDGN-G9PQG-XVVXX-R3X43-63DFG'
            }
            'Windows Server 2019 Standard Evaluation' {
                $KMSClientkey = 'N69G4-B89J2-4G8F4-WWYCC-J464C'
            }
            'Windows Server 2019 Standard' {
                $KMSClientkey = 'N69G4-B89J2-4G8F4-WWYCC-J464C'
            }

        }
        & DISM.exe /Online /Set-Edition:$target_edition /ProductKey:$KMSClientkey /AcceptEula /NoRestart
        CheckRebootAndContinue 
    }
    else {
        Write-Output 'OS Edition is OK'
    }
}

function InstallSQLServer {
    param(
        [string]$SQLServerVersion
    )
    $installed = Test-Path "C:\Program Files\Microsoft SQL Server"
    switch ($SQLServerVersion) {
        '2016sp2ent' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Ent_Core_2016w_SP2_64Bit_English_OEM_VL_X21-59533.ISO"
            $SQLServerKey = "TBR8B-BXC4Y-298NV-PYTBY-G3BCP"
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL13.MSSQLSERVER\MSSQL\Data"'
        }
        '2016sp2std' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Ent_Core_2016w_SP2_64Bit_English_OEM_VL_X21-59533.ISO"
            $SQLServerKey = "B9GQY-GBG4J-282NY-QRG4X-KQBCR"
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL13.MSSQLSERVER\MSSQL\Data"'
        }
        '2016sp2dev' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Ent_Core_2016w_SP2_64Bit_English_OEM_VL_X21-59533.ISO"
            $SQLServerKey = "22222-00000-00000-00000-00000"
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL13.MSSQLSERVER\MSSQL\Data"'
        }
        '2019ent' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Ent_Core_2019Dec2019_64Bit_English_OEM_VL_X22-22120.ISO"
            $SQLServerKey = ""
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL15.MSSQLSERVER\MSSQL\Data"'
        }
        '2019std' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Standard_Edtn_2019Dec2019_64Bit_English_OEM_VL_X22-22109.ISO"
            $SQLServerKey = ""
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL15.MSSQLSERVER\MSSQL\Data"'
        }
        '2019dev' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SQLServer2019-x64-ENU-Dev.iso"
            $SQLServerKey = "22222-00000-00000-00000-00000"
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL15.MSSQLSERVER\MSSQL\Data"'
        }
        '2017ent' {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Ent_Core_2017_64Bit_English_OEM_VL_X21-56995.ISO"
            $SQLServerKey = ""
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL14.MSSQLSERVER\MSSQL\Data"'
        }
        "2017std" {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Standard_Edtn_2017_64Bit_English_OEM_VL_X21-56945.ISO"
            $SQLServerKey = ""
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL14.MSSQLSERVER\MSSQL\Data"'
        }
        "2017dev" {
            $SQLServerURL = "https://mdb-windows.s3.mds.yandex.net/SW_DVD9_NTRL_SQL_Svr_Standard_Edtn_2017_64Bit_English_OEM_VL_X21-56945.ISO"
            $SQLServerKey = "22222-00000-00000-00000-00000"
            $DBDir = 'SQLUSERDBDIR="D:\SqlServer\MSSQL14.MSSQLSERVER\MSSQL\Data"'
        }
        default {
            throw "Unknown SQLServer version: $SQLServerVersion"
        }
    }
    If (-Not $installed) {
        Write-Host "Downloading MS SQL Server"
        $SQLServerISO = "$ENV:TEMP\Downloads\sqlserver.iso"
        DownloadAndVerify $SQLServerURL $SQLServerISO
        Mount-DiskImage -ImagePath $SQLServerISO
        
	If ($SQLServerKey) {
        Add-Content -Path "A:\sqlserver_install.ini" -Value "PID=""$SQLServerKey"""
    }
    
    Add-Content -Path "A:\sqlserver_install.ini" -Value $DBDir

	Add-Type -AssemblyName System.Web
	$saPwd = [System.Web.Security.Membership]::GeneratePassword(64,2)
	"SAPWD=$saPwd\r\n" | Add-Content -Path "A:\sqlserver_install.ini"

    $ArgumentList = "/Q /ACTION=install /UpdateEnabled=True /CONFIGURATIONFILE=A:\sqlserver_install.ini"

    Write-Host "Installing MS SQL Server"
    Start-Process -Wait -FilePath "E:\setup.exe" -ArgumentList $ArgumentList
		
	Write-Host "Archiving data disk"
	Get-Service | Where { $_.Name -like '*SQL*' } | Stop-Service -Force
	Compress-Archive -Path D:\SqlServer -DestinationPath C:\datadir.zip
    }
}

function PatchWin_Dacl {
    $win_dacl = "C:\salt\bin\Lib\site-packages\salt-3000.9-py2.7.egg\salt\utils\win_dacl.py"
    $patched = ((Get-Content -Path $win_dacl|select-string "# Microsoft introduced the concept of Capability SIDs in Windows 8"|Measure-Object).Count -gt 0)
    if (-not($patched)){
        Write-host "Patching win_dacl"
        cp A:\win_dacl.py $win_dacl
    }
    Write-host "Win_dacl patched"
}

########### MDB: Install SQL Server ODBC driver 17
function UpdateODBC {
    $installed = (Get-odbcdriver -Platform 64-bit | where-object { $_.Name -like 'ODBC Driver 17 for SQL Server' })
    If (-Not $installed) {
        # Download
        Write-Host "Downloading ODBC Driver 17"
        $OdbcURL = 'https://mdb-windows.s3.mds.yandex.net/msodbcsql.msi'
        $OdbcMSIPath = "$ENV:TEMP\Downloads\msodbcsql.msi"
        DownloadAndVerify $OdbcURL $OdbcMSIPath

        # Installing
        Write-Host "Installing ODBC Driver 17"
        Start-Process -FilePath 'msiexec.exe' -ArgumentList "/qn /i $OdbcMSIPath IACCEPTMSODBCSQLLICENSETERMS=YES" -Wait -Passthru
    }
    else {
        Write-Host "ODBC driver 17 for SQL Server is installed."
    }
}


try {
    # UNSAFE: following 5 commands, can't be detected via Bootstrap.err file
    # they should just works fine
    InstallVirtioDrivers
    SetupNetInterface
    SetupHostname
    PrepareDownloads
    InstallOpenSSH
    CreateTempBuildUser
    # /UNSAFE

    InstallPoweshell7
    ConfigureTimeService
    InstallFeatures 'Failover-Clustering'
    InstallVim
    InstallVisualCRuntime
    InstallMdbConfigSalt
    InstallSaltMinion
    PatchWin_Dacl
    UpdatePipFromS3
    if ($product -eq 'sqlserver'){
        InstallPyodbc
        }
    UpdatePackageManagement
    ApplyHighstate $suite
    if ($datadisk_required){
        PrepareDataDisk
        }
    if ($product -eq 'sqlserver'){
        InstallSQLServer $product_version
        UpdateODBC
        InstallDbaTools
        }
    #InstallSSMS
    InstallYAMLModule
    InstallSetupScripts
    InstallFar
    SetPowerPlan
    GrantRunAsAService
    UpgradeEdition 
    InstallWindowsUpdates
    EnableSerialConsole
    HardenWindows
    Cleanup
    UpdatePassword
    #EnableWinRM
    # Run Sysprep
    & "$ENV:SystemRoot\System32\Sysprep\Sysprep.exe" /generalize /oobe /shutdown /unattend:"A:\SysprepUnattend.xml"
}
catch {
    Set-Content C:\Bootstrap.error "ERROR"
}
