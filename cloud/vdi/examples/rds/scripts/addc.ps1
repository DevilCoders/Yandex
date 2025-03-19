Start-Transcript -OutputDirectory "$ENV:SystemDrive\Log\" -Force
$SerialPort = New-Object System.IO.Ports.SerialPort('COM1')
$script:SerialPortStaleMessages = @()

##########
# helpers
##########

filter Out-Info {
    $j = New-JsonMessage -Message $_ -Type "log"
    Out-Message $j
}

filter Out-Fatal {
    $m = $_
    # prettify if got ErrorRecord object
    if ($m.GetType().name -eq 'ErrorRecord') {
        $m = "at line: $($PSItem.InvocationInfo.ScriptLineNumber) char: $($PSItem.InvocationInfo.OffsetInLine): got error: $($PSItem.ToString())"
    }

    $j = New-JsonMessage -Message $m -Type "fatal"
    Out-Message $j
    Get-ScheduledTask "deploy" | Disable-ScheduledTask
    Exit 1
}

filter Out-Error {
    $m = $_
    # prettify if got ErrorRecord object
    if ($m.GetType().name -eq 'ErrorRecord') {
        $m = "at line: $($PSItem.InvocationInfo.ScriptLineNumber) char: $($PSItem.InvocationInfo.OffsetInLine): got error: $($PSItem.ToString())"
    }

    $j = New-JsonMessage -Message $m -Type "error"
    Out-Message $j
}

function Out-Message {
    param(
        [ValidateScript({-not [String]::isNullOrEmpty($_)})]
        [String]$Message
    )

    $Message | Out-Default
    
    if ($SerialPort.IsOpen) {
        # flush some messages, which could occur before we occupy serial port
        if ($script:SerialPortStaleMessages.Length -gt 0) {
            foreach ($OldMessage in $SerialPortStaleMessages) {
                $SerialPort.WriteLine($OldMessage)
            }
            $script:SerialPortStaleMessages = @()
        }

        $SerialPort.WriteLine($Message)
    } else {
        $script:SerialPortStaleMessages += $Message #.Clone()
    }
}

function New-JsonMessage {
    param(
        [ValidateScript({-not [String]::isNullOrEmpty($_)})]
        [String]$Message,

        [ValidateSet("log", "error", "fatal")]
        [string]$Type
    )
    $d = Get-Date
    $e = ConvertTo-Unixtime -Date $d
  
    $o = [pscustomobject][ordered]@{
        timestamp = $e;
        type = $type;
        id = [guid]::NewGuid().Guid;
        payload = [pscustomobject][ordered]@{
            date = $d.ToString();
            msg = $message;
        };
    }
  
    return ($o | ConvertTo-Json -Compress)
}

function ConvertTo-Unixtime {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateNotNull()]
        [datetime]$Date
    )

    return [int64]([double]::Parse((Get-Date -Date ($Date.ToUniversalTime()) -UFormat "%s"),[CultureInfo][System.Threading.Thread]::CurrentThread.CurrentCulture))
}

##########
# init
##########

# wait till setupcomplete finishes and delete itself
try {
    $Timeout = New-TimeSpan -Minutes 3
    $Stopwatch = New-Object -TypeName System.Diagnostics.Stopwatch
    $Stopwatch.Start()
    $SetupcompleteFilepath = "$Env:windir\setup\scripts\SetupComplete.cmd"
    while (Test-Path $SetupcompleteFilepath) {
        "Waiting 10s SetupComplete to finish" | Out-Info
        Start-Sleep -Seconds 10

        if ($Stopwatch.Elapsed -gt $Timeout) {
            throw "setupcomplete were running to long($($Timeout.TotalMinutes)+ minutes), giving up..."
        }
    }
} catch {
    $PSItem | Out-Fatal
} finally {
    $Stopwatch.Stop()
    $Stopwatch.Reset()
}

# wait till cloudbase-init finish it's job
try {
    $Timeout = New-TimeSpan -Minutes 3
    $Stopwatch = New-Object -TypeName System.Diagnostics.Stopwatch
    $Stopwatch.Start()
    while ((Get-Service 'cloudbase-init').Status -eq 'Running') {
        "Waiting 10s cloudbase-init to stop" | Out-Info
        Start-Sleep -Seconds 10

        if ($Stopwatch.Elapsed -gt $Timeout) {
            throw "cloudbase-init service were running to long ($($Timeout.TotalMinutes)+ minutes), giving up..."
        }
    }
} catch {
    $PSItem | Out-Fatal
} finally {
    $Stopwatch.Stop()
    $Stopwatch.Reset()
}

# wait till COM1 port is availible
try {
    $Timeout = New-TimeSpan -Minutes 3
    $Stopwatch = New-Object -TypeName System.Diagnostics.Stopwatch
    $Stopwatch.Start()
    while (-not $SerialPort.IsOpen) {
        "Waiting 10s COM1 port to become availible" | Out-Info
        Start-Sleep -Seconds 1

        if ($Stopwatch.Elapsed -gt $Timeout) {
            throw "COM1 port was busy to long ($($Timeout.TotalMinutes)+ minutes), giving up..."
        }
        try { 
            $SerialPort.Open()
        } catch [UnauthorizedAccessException]{
            $PSItem | Out-Error
        }
    }
    $SerialPort.WriteLine("")
} catch {
    $PSItem | Out-Fatal
} finally {
    $Stopwatch.Stop()
    $Stopwatch.Reset()
}

##########
# deployment
##########

# install role, create forest, configure network interface and set dns server forwarder
$isADDSRoleInstalled = (Get-WindowsFeature -Name AD-Domain-Services).Installed
if (-not $isADDSRoleInstalled) {
    try {
        "install role" | Out-Info
        Install-WindowsFeature -Name AD-Domain-Services -IncludeManagementTools
    } catch {
        $PSItem | Out-Fatal
    }

    $PlainSafeModeAdministratorPassword = '${ recovery_password }'
    $param = @{
        String = $PlainSafeModeAdministratorPassword
        AsPlainText = $true
        Force = $true
    }
    try {
        $SafeModeAdministratorPassword = ConvertTo-SecureString @param
    } catch {
        $PSItem | Out-Fatal
    }

    $DomainName = '${ domain_name }'
    $param = @{
        ForestMode                    = "WinThreshold"
        DomainMode                    = "WinThreshold"
        DomainName                    = $DomainName
        DomainNetbiosName             = $DomainName.Split(".")[0]
        DatabasePath                  = "C:\Windows\NTDS"
        SysvolPath                    = "C:\Windows\SYSVOL"
        LogPath                       = "C:\Windows\NTDS"
        InstallDns                    = $true
        CreateDnsDelegation           = $false
        NoRebootOnCompletion          = $true
        Force                         = $true
        SafeModeAdministratorPassword = $SafeModeAdministratorPassword
    }
    try {
        "create forest" | Out-Info
        Install-ADDSForest @param
    } catch {
        $PSItem | Out-Fatal
    }

    try {
        $DNSForwarder = "${ dns_forwarder }"
        "configure dns forwarder" | Out-Info
        Set-DnsServerForwarder $DNSForwarder
    } catch {
        $PSItem | Out-Fatal
    }

    "restart" | Out-Info
    Restart-Computer -Force
} else {
    # check if we could find and connect to active directory domain after reboot
    $DomainName = '${ domain_name }'
    $UserName   = "Administrator"
    $Password   = '${ admin_password }'
    
    try {
        $Timeout = New-TimeSpan -Minutes 10
        $Stopwatch = New-Object -TypeName System.Diagnostics.Stopwatch
        $Stopwatch.Start()

        $Context = New-Object System.DirectoryServices.ActiveDirectory.DirectoryContext('Domain', $DomainName, $UserName, $Password)
        $found = $false
        while (-not $found) {
            Clear-DnsClientCache

            try {
                $found = $null -ne [System.DirectoryServices.ActiveDirectory.DomainController]::FindOne($Context)
            } catch [System.DirectoryServices.ActiveDirectory.ActiveDirectoryObjectNotFoundException] {
                "Waiting 10s for ADDS..." | Out-Info
                Start-Sleep -Seconds 10
            }

            if ($Stopwatch.Elapsed -gt $Timeout) {
                throw "ADDS service were unavailable to long ($($Timeout.TotalMinutes)+ minutes), giving up..."
            }
        }
    } catch {
        $PSItem | Out-Fatal
    }

    "deployment complete" | Out-Info
    Unregister-ScheduledTask -TaskName 'deploy' -Confirm:$false
}
