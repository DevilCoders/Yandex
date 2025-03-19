function Get-InstanceMetadata ($SubPath) {
  $Headers = @{"Metadata-Flavor" = "Google"}
  $Url = "http://169.254.169.254/computeMetadata/v1/instance" + $SubPath

  return Invoke-RestMethod -Headers $Headers $Url
}

function Get-InstanceEthIndexes {
  return (Get-InstanceMetadata -SubPath "/network-interfaces/") -replace "/"
}

function Get-InstanceEthMacAddress ($index) {
  return Get-InstanceMetadata -SubPath "/network-interfaces/$index/mac"
}

function Get-InstanceHostname {
  return Get-InstanceMetadata -SubPath "/hostname"
}

function Get-InstanceName {
  return Get-InstanceMetadata -SubPath "/name"
}

function Get-RebootRequiredCode {
  $isRequired = (Get-CBSRebootReuired) -or `
                (Get-WAURebootReuired) -or `
                (Get-PendingRenameRebootReuired)
  if ($isRequired) { 
    return 3010
  } else {
    return 0
  }
}

function Get-CBSRebootReuired {
  if (Get-ChildItem "HKLM:Software\Microsoft\Windows\CurrentVersion\Component Based Servicing\RebootPending" -ErrorAction:SilentlyContinue) {
    return $true
  } else {
    return $false
  }
}

function Get-WAURebootReuired {
  if (Get-ChildItem "HKLM:SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate\Auto Update\RebootRequired" -ErrorAction:SilentlyContinue) {
    return $true
  } else {
    return $false
  }
}

function Get-PendingRenameRebootReuired {
  if (Get-ItemProperty "HKLM:SYSTEM\CurrentControlSet\Control\Session Manager" -Name PendingFileRenameOperations -ErrorAction:SilentlyContinue) {
    return $true
  } else {
    return $false
  }
}

function Clear-DownloadedUpdates {
  # Removing downloaded updates
  Stop-Service wuauserv -Force
  Remove-Item "$ENV:windir\SoftwareDistribution\*" -ErrorAction:SilentlyContinue -Recurse
  Start-Service wuauserv
}

function Clear-PantherLogs {
  # A bit odd, but ok
  Remove-Item -Path "$ENV:windir\PANTHER\*" -filter "*.log" -ErrorAction:SilentlyContinue -Recurse
  Remove-Item -Path "$ENV:windir\PANTHER\*" -filter "*.xml" -ErrorAction:SilentlyContinue -Recurse
}

function Clear-TempFiles {
  # mb clear 'admnistrator' temp also ?
  Remove-Item -Path "$ENV:windir\TEMP\*" -Exclude "*.ps1" -ErrorAction:SilentlyContinue -Recurse
  Remove-Item -Path "$ENV:TEMP\*" -ErrorAction:SilentlyContinue -Recurse
  Remove-Item -Path "$ENV:TMP\*" -ErrorAction:SilentlyContinue -Recurse
}

function Clear-Prefetch {
  # unlikely to happen but still
  Remove-Item -Path "$env:windir\Prefetch\*" -ErrorAction:SilentlyContinue -Recurse
}

function Clear-WER {
  # very unlikely to happen but still
  Remove-Item -Path "$ENV:ProgramData\Microsoft\Windows\WER\ReportArchive\*" -ErrorAction:SilentlyContinue -Recurse
  Remove-Item -Path "$ENV:ProgramData\Microsoft\Windows\WER\ReportQueue\*" -ErrorAction:SilentlyContinue -Recurse
  Remove-Item -Path "$ENV:ProgramData\Microsoft\Windows\WER\Temp\*" -ErrorAction:SilentlyContinue -Recurse
}

function Clear-WindowsSystemLogs {
  # another odd place
  Get-ChildItem "$ENV:SystemRoot\Logs\*" -File -Recurse | Remove-Item -ErrorAction:SilentlyContinue
}

function Clear-EventLogs {
  # theese two 'eventlogs' always crits, l8r need to look at'em
  & wevtutil el | `
    Where-Object { ($_ -ne 'Microsoft-Windows-LiveId/Analytic') `
              -and ($_ -ne 'Microsoft-Windows-LiveId/Operational') `
              -and ($_ -ne 'Microsoft-Windows-TerminalServices-Licensing/Analytic') `
              -and ($_ -ne 'Microsoft-Windows-TerminalServices-Licensing/Debug') } | `
      Foreach-Object { wevtutil cl "$_" }
}

function Clear-CleanMgr {
  # ToDo: mark more granually, very long and frustrating cleanup, but fancy, could clean lot of 'places of interests'

  # mark each option with key-flag-name-of-our-choice
  foreach ($Key in (Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\" | Select-Object -ExpandProperty Name)) {
    New-ItemProperty `
      -Path "$Key" `
      -Name "StateFlags1234" `
      -Value 2 `
      -PropertyType DWord `
      -ErrorAction:SilentlyContinue
  }
  
  # run cleanup
  Start-Process -FilePath "cleanmgr.exe" -ArgumentList "/sagerun:1234" -Wait -ErrorAction:SilentlyContinue
  
  # clean our modification
  foreach ($Key in (Get-ChildItem "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\" | Select-Object -ExpandProperty Name)) {
    Remove-ItemProperty `
      -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\VolumeCaches\$Key" `
      -Name StateFlags1234 -ErrorAction SilentlyContinue `
  }

  Get-Process -Name cleanmgr,dismhost -ErrorAction:SilentlyContinue | Wait-Process 
}

function Clear-WinSxSComponents {
  # echo of war
  Start-Process -FilePath 'dism.exe' -ArgumentList "/Online /Cleanup-Image /StartComponentCleanup /ResetBase" -Wait
}

function Clear-VSS {
  # do we really need it? will think l8r
  Start-Process -FilePath 'vssadmin.exe' -ArgumentList "delete shadows /all /quiet" -Wait
}

function Clear-FragmentedSpace {
  # from NBS 'point of view' is stupid?
  #Optimize-Volume -DriveLetter C -Defrag
  Get-ScheduledTask -TaskName 'ScheduledDefrag' | Disable-ScheduledTask | Out-Null
}

function Clear-TaskSchedulerRemnants {
  # packer bug in some version, run via packer is 'distructive'
  Get-ScheduledTask -TaskPath "\" -ErrorAction:SilentlyContinue | `
    ? TaskName -like "packer-*" | `
      Unregister-ScheduledTask -Confirm:$false -ErrorAction:SilentlyContinue
}

# add debug output?
# there is parser bug i guess - you have to have no empty lines in class
# wrote only to pass 'context' between functions, but could be sort of example
class WindowsUpdates {
  hidden $__updates
  #hidden $__sysinfo
  hidden $__session
  hidden $__searcher
  hidden $__downloader
  hidden $__installer
  hidden $__impact_description = @{
    "0" = "Normal";
    "1" = "Minor";
    "2" = "RequiresExclusiveHandling"
  }
  hidden $__reboot_behavior_description = @{
    "0" = "NeverReboots"
    "1" = "AlwaysRequiresReboot"
    "2" = "CanRequestReboot"
  }
  hidden $__search_result_description = @{
    "0" = "NotStarted"
    "1" = "InProgress"
    "2" = "Succeeded"
    "3" = "SucceededWithErrors"
    "4" = "Failed"
    "5" = "Aborted"
  }
  hidden [string] __get_description ([string]$code, [hashtable]$description) {
    try {
      return $description[$code]
    } catch {
      return "unknown"
    }    
  }
  hidden [string] __get_impact_description ([string]$code) {
    return $this.__get_description($code, $this.__impact_description)
  }
  hidden [string] __get_reboot_behavior_description ([string]$code) {
    return $this.__get_description($code, $this.__reboot_behavior_description)
  }
  hidden [string] __get_common_result_description ([string]$code) {
    return $this.__get_description($code, $this.__search_result_description)
  }
  WindowsUpdates () {
    $this.__updates = New-Object -ComObject "Microsoft.Update.UpdateColl"
    #$this.__sysinfo = New-Object -ComObject 'Microsoft.Update.SystemInfo'
    $this.__session = New-Object -ComObject "Microsoft.Update.Session"    
    $this.__searcher = $this.__session.CreateUpdateSearcher()
    $this.__searcher.ServerSelection = 2 # windows updates source
    $this.__searcher.SearchScope = 1 # machine only
    $this.__downloader = $this.__session.CreateUpdateDownloader()
    $this.__downloader.Priority = 4 # highest download priority, supported in ws2012r2 for sure
    $this.__installer = $this.__session.CreateUpdateInstaller()
  }
  [void] Find () {
    $search = $this.__searcher.search("Type='Software' and IsInstalled = 0 and IsHidden = 0")
    $search_result_description = $this.__get_common_result_description($search.resultCode)
    if ("Succeeded" -eq $search_result_description) {
      # write-debug/verbose?
      #Write-Host "Search succeeded with `"$($search_result_description)`", code `"$($search.resultCode)`""
      foreach ($update in $search.updates) {
        $update.AcceptEula() | Out-Null
        $this.__updates.Add($update)
      }
    } else {
      # error at search, throw something?
      # write-debug/verbose?
      #Write-Host "Search failed with `"$($search_result_description)`", code `"$($search.resultCode)`""
      $this.__updates.clear()
    }
  }
  [void] Download () {
    $this.__downloader.Updates = $this.__updates
    $download = $this.__downloader.Download()
    $download_result_description = $this.__get_common_result_description($download.resultCode)
    if ("Succeeded" -eq $download_result_description) {
      # write-debug/verbose?
      #Write-Host "Download succeeded with `"$($download_result_description)`", code `"$($download.resultCode)`""
    } else {
      # error at download, throw something?
      # write-debug/verbose?
      #Write-Host "Download failed with `"$($download_result_description)`", code `"$($download.resultCode)`""
    }
  }
  [void] Install () {
    # maybe install one by one? or use async api?
    $this.__installer.Updates = $this.__updates
    # check RebootRequiredBeforeInstallation ?
    $installation = $this.__installer.Install()
    $installation_result_description = $this.__get_common_result_description($installation.resultCode)
    if ("Succeeded" -eq $installation_result_description) {
      # write-debug/verbose?
      #Write-Host "Install succeeded with `"$($installation_result_description)`", code `"$($installation.resultCode)`""
    } else {
      # error at download, throw something?
      # write-debug/verbose?
      #Write-Host "Install failed with `"$($installation_result_description)`", code `"$($installation.resultCode)`""
    }
  }
  #[bool] RebootRequired () {
  #  $required = $false
  #  $sysinfo = New-Object -ComObject 'Microsoft.Update.SystemInfo'
  #  $required = $required -or $sysinfo.RebootRequired
  #  # write-debug/verbose?
  #  Write-Host "Sysinfo.RebootRequired = $($sysinfo.RebootRequired)"
  #}
  [array] Show () {
    # json as output?
    $result = @()
    foreach ($update in $this.__updates) {
      $ImpactCode = $update.InstallationBehavior.Impact
      $RebootBehaviorCode = $update.InstallationBehavior.RebootBehavior
      $result += [pscustomobject][ordered]@{
        Title = $update.Title;
        KBArticleIDs = "KB$($update.KBArticleIDs)";
        Date = $update.LastDeploymentChangeTime.ToString("dd-MM-yyyy");
        SizeMB = ($update.MaxDownloadSize / 1MB).ToString("0.#");
        SupportUrl = $update.SupportUrl;
        EulaAccepted = $update.EulaAccepted;
        BrowseOnly = $update.BrowseOnly;
        AutoSelectOnWebSites = $update.AutoSelectOnWebSites;
        Severity = $update.MsrcSeverity;
        Impact = $this.__get_impact_description($ImpactCode);
        RebootBehavior = $this.__get_reboot_behavior_description($RebootBehaviorCode);
        RebootRequired = $update.RebootRequired;
        CanRequestUserInput = $update.InstallationBehavior.CanRequestUserInput
      }
    }
    return $result
  }
}

# sort of if __main__ == "__main__"
function Test-DotSourced {
  return ($MyInvocation.InvocationName -eq '.' -or $MyInvocation.Line -eq '')
}

function Test-WindowsUpdateReachable {
  return Test-Connection 'download.microsoft.com' -Count 1 -Quiet
}

function Wait-WindowsUpdateReachable ($Retries = 10, $backoff = 5) {
  do {
    Start-Sleep -Seconds $backoff # make it exponential
  } while (-not (Test-WindowsUpdateReachable) -and $Retries -gt 0)
}

# yup ngen'em
function Get-NgenImagesPath {
  return Get-ChildItem $ENV:windir\Microsoft.Net -Filter ngen.exe -Recurse | `
    Select-Object -ExpandProperty FullName
}

#function Run-NgenUpdates ($Ngen) {
#  $response = & $Ngen update /force /queue
#}

# update powershell
function Get-NgenImage {
  return "$([Runtime.InteropServices.RuntimeEnvironment]::GetRuntimeDirectory())\ngen.exe"
}

function Get-PowershellAssemblies {
  return [AppDomain]::CurrentDomain.GetAssemblies() | `
    Where-Object location -like '*\Microsoft.PowerShell*'
}

#function Run-NgenInstalls ($Ngen, $Path) {
#  $response = & $Ngen install $Path
#}

# to download from s3 without external libs/bins

function Get-WindowsVersion {
  return Get-ItemProperty `
    -Path 'HKLM:\Software\Microsoft\Windows NT\CurrentVersion' `
    -Name ProductName | `
      Select-Object -ExpandProperty ProductName
}

function Get-KMSClientKey {
  switch (Get-WindowsVersion) {
    'Windows Server 2012 R2 Datacenter' {
      $KMSClientkey = 'W3GGN-FT8W3-Y4M27-J84CP-Q3VJ9'
    }
    'Windows Server 2016 Datacenter' {
      $KMSClientkey = 'CB7KF-BWN84-R7R2Y-793K2-8XDDG'
    }
    'Windows Server 2019 Datacenter' {
      $KMSClientkey = 'WMDGN-G9PQG-XVVXX-R3X43-63DFG'
    }
    'Windows Server 2012 R2 Standard' {
      $KMSClientkey = 'D2N9P-3P6X9-2R39C-7RTCD-MDVJX'
    }
    'Windows Server 2016 Standard' {
      $KMSClientkey = 'WC2BQ-8NRM3-FDDYY-2BFGV-KHKQY'
    }
    'Windows Server 2019 Standard' {
      $KMSClientkey = 'N69G4-B89J2-4G8F4-WWYCC-J464C'
    }
  }

  return $KMSClientkey
}

function Get-LicenseInfo {
  return & cscript /nologo $ENV:windir\system32\slmgr.vbs /dli
}

function Get-PartialProductKey {
  return ((Get-LicenseInfo | sls "Partial Product Key:") -split " ")[-1]
}

function Get-KMSServer {
  return ((Get-LicenseInfo | Select-String "Registered KMS machine name:") -split " ")[-1]
}

# ToDo: write fancy response
function Set-KMSServer ($KMSServerName) {
  $response = & cscript /nologo $ENV:windir\system32\slmgr.vbs /skms $KMSServerName
}

# ToDo: write fancy response
function Set-KMSClientKey { 
  $response = & cscript /nologo $ENV:windir\system32\slmgr.vbs /ipk Get-KMSClientKey
}

function Get-WindowsActivationStatus {
  return (& cscript /nologo C:\Windows\system32\slmgr.vbs /dli | Select-String -Pattern '^License Status:') -match 'Licensed'
}

function Invoke-WindowsActivation {
  $r = & cscript /nologo $ENV:windir\system32\slmgr.vbs /ato | `
    Select-String 'Product activated successfully'

  return (-not ([string]::IsNullOrEmpty($r)))
}

function Set-EnvironmentVariable ($Name, $Value) {
  [Environment]::SetEnvironmentVariable($Name, $Value, "Machine")
}

function Get-EnvironmentVariables {
  [Environment]::GetEnvironmentVariables("Machine")
}

function Get-EnvironmentVariable ($Name) {
  [Environment]::GetEnvironmentVariable($Name, "Machine")
}

function Get-InstanceMetadata ($SubPath) {
  $Headers = @{"Metadata-Flavor" = "Google"}
  $Url = "http://169.254.169.254/computeMetadata/v1/instance" + $SubPath

  return Invoke-RestMethod -Headers $Headers $Url
}

function Get-InstanceMetadataEthIndexes {
  return (Get-InstanceMetadata -SubPath "/network-interfaces/") -replace "/"
}

function Get-InstanceMetadataEthMacAddress ($index) {
  return Get-InstanceMetadata -SubPath "/network-interfaces/$index/mac"
}

function Get-InstanceMetadataHostname {
  return Get-InstanceMetadata -SubPath "/hostname"
}

function Get-InstanceMetadataName {
  return Get-InstanceMetadata -SubPath "/name"
}

function Remove-MyCertificate ($CertificateThumbPrint) {
  Remove-Item -Path "Cert:\LocalMachine\My\$CertificateThumbPrint" | Out-Null
}

function Clear-MyCertificates {
  Remove-Item -Path Cert:\LocalMachine\My\* | Out-Null
}

function Clear-MyCertificatesWithoutPK {
  ls Cert:\LocalMachine\My\* | ? HasPrivateKey -eq $false | Remove-Item | Out-Null
}

function Get-WinrmHTTPListener {
  return Get-ChildItem WSMan:\localhost\Listener\ | `
    ? Keys -contains 'Transport=HTTP' | `
      Select-Object -ExpandProperty Name
}

function Get-WinrmHTTPSListener {
  return Get-ChildItem WSMan:\localhost\Listener\ | `
    ? Keys -contains 'Transport=HTTPS' | `
      Select-Object -ExpandProperty Name
}

function Get-WinrmCertificate {
  $Listner = Get-WinrmHTTPSListener
  return Get-ChildItem "WSMan:\localhost\Listener\$Listner\CertificateThumbPrint" | `
    Select-Object -ExpandProperty Value
}

function New-WinrmCertificate {
  $DnsName = [System.Net.Dns]::GetHostByName($env:computerName).Hostname
  $WindowsVersion = Get-WindowsVersion
  if ($WindowsVersion -match "Windows Server 2012 R2*") {
    # yup, black sheep
    return New-SelfSignedCertificate `
      -CertStoreLocation "Cert:\LocalMachine\My" `
      -DnsName $DnsName
  } else {
    return New-SelfSignedCertificate `
      -CertStoreLocation "Cert:\LocalMachine\My" `
      -DnsName $DnsName `
      -Subject $ENV:COMPUTERNAME
  }
}

function New-WinrmHTTPSListener ($CertificateThumbPrint) {
  New-Item -Path WSMan:\LocalHost\Listener `
    -Transport HTTPS `
    -Address * `
    -CertificateThumbPrint $CertificateThumbprint `
    -HostName $ENV:COMPUTERNAME `
    -Force | Out-Null
}

function New-WinrmHTTPListener {
  New-Item -Path WSMan:\LocalHost\Listener `
    -Transport HTTP `
    -Address * `
    -Force | Out-Null
}

function Enable-WinrmHTTPFirewallRule {
  if ( $WINRMHTTP = Get-NetFirewallRule -Name "WINRM-HTTP-In-TCP" -ErrorAction SilentlyContinue ) {
    $WINRMHTTP | Enable-NetFirewallRule
  } else {
    New-NetFirewallRule `
      -Group "Windows Remote Management" `
      -DisplayName "Windows Remote Management (HTTP-In)" `
      -Name "WINRM-HTTP-In-TCP" `
      -LocalPort 5985 `
      -Action "Allow" `
      -Protocol "TCP" `
      -Program "System"
  }
}

function Enable-WinrmHTTPSFirewallRule {
  if ($WINRMHTTPS = Get-NetFirewallRule -Name "WINRM-HTTPS-In-TCP" -ErrorAction SilentlyContinue) {
    $WINRMHTTPS | Enable-NetFirewallRule  
  } else {
    New-NetFirewallRule `
      -Group "Windows Remote Management" `
      -DisplayName "Windows Remote Management (HTTPS-In)" `
      -Name "WINRM-HTTPS-In-TCP" `
      -LocalPort 5986 `
      -Action "Allow" `
      -Protocol "TCP" `
      -Program "System"
  }  
}

function Enable-WinrmFirewallRules {
  Enable-WinrmHTTPFirewallRule
  Enable-WinrmHTTPSFirewallRule
}

function Clear-WinrmListeners {
  Remove-Item -Path WSMan:\Localhost\listener\listener* -Recurse | Out-Null
}
