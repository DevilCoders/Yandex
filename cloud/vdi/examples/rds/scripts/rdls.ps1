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
        [Parameter(Mandatory=$true)]
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
        [Parameter(Mandatory=$true)]
        [ValidateScript({-not [String]::isNullOrEmpty($_)})]
        [String]$Message,

        [Parameter(Mandatory=$true)]
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

function Wait-ADDomain {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateScript({-not [String]::isNullOrEmpty($_)})]
        [String]$DomainName,

        [Parameter(Mandatory=$true)]
        [PSCredential]$Credential
    )

    $Timeout = New-TimeSpan -Minutes 15
    $Stopwatch = New-Object -TypeName System.Diagnostics.Stopwatch
    $Stopwatch.Start()

    $Context = New-Object System.DirectoryServices.ActiveDirectory.DirectoryContext('Domain', $DomainName, $Credential.UserName, $Credential.GetNetworkCredential().Password)
    $found = $false

    while (-not $found) {
        Clear-DnsClientCache

        try {
            $found = $null -ne [System.DirectoryServices.ActiveDirectory.DomainController]::FindOne($Context)
        } catch {
            "Waiting 10s for ADDS..." | Out-Info
            Start-Sleep -Seconds 10
        }

        if ($Stopwatch.Elapsed -gt $Timeout) {
            throw "ADDS service were unavailable to long ($($Timeout.TotalMinutes)+ minutes), giving up..."
        }
    }
}

function Approve-RDLServer {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateScript({-not [String]::isNullOrEmpty($_)})]
        [String]$DomainName,

        [Parameter(Mandatory=$true)]
        [PSCredential]$Credential
    )

    # since everyting is executed as SYSTEM and we dont want to bring AD RSAT...
    $DomainDN       = 'DC=' + ($DomainName -replace '\.', ',DC=')
    $DirectoryEntry = New-Object System.DirectoryServices.DirectoryEntry("LDAP://$DomainName/$DomainDN", $Credential.UserName, $Credential.GetNetworkCredential().Password)
    $Searcher       = New-Object System.DirectoryServices.DirectorySearcher($DirectoryEntry)

    # get the group distinguished name
    $GroupName       =  'Terminal Server License Servers'
    $Searcher.Filter = "(&(objectCategory=Group)(cn=$GroupName))"
    $GroupDN         = $Searcher.FindOne().Properties['distinguishedname']

    # check to see if computer is already a member of the group
    $ComputerName   = $env:COMPUTERNAME
    $Searcher.Filter = "(&(objectCategory=Computer)(objectClass=User)(samaccountname=$ComputerName$)(memberof=$GroupDN))"
    $isMember = [bool]$Searcher.FindOne()
    if ($isMember) { 
        return
    }

    # get computer object
    $Searcher.Filter = "(&(objectCategory=Computer)(objectClass=User)(samaccountname=$ComputerName$))"
    $ComputerDN      = $Searcher.FindOne().Properties['distinguishedname']
    $ComputerObject  = New-Object System.DirectoryServices.DirectoryEntry("LDAP://$DomainName/$ComputerDN", $Credential.UserName, $Credential.GetNetworkCredential().Password)

    # add computer to the group
    $GroupObject = New-Object System.DirectoryServices.DirectoryEntry("LDAP://$DomainName/$GroupDN", $Credential.UserName, $Credential.GetNetworkCredential().Password)
    $GroupObject.Add($ComputerObject.AdsPath)
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

# check if we could find and connect to active directory domain
$PlainPassword = '${ admin_password }'
$DomainName    = '${ domain_name }'
$Password      = ConvertTo-SecureString -String $PlainPassword -AsPlainText -Force
$Credential    = New-Object PSCredential("$DomainName\Administrator", $Password)

try {
    "check and wait for active directory" | Out-Info
    Wait-ADDomain -DomainName $DomainName -Credential $Credential
} catch {
    $PSItem | Out-Fatal
}

# add machine into domain
"check domain membership" | Out-Info
$ComputerSystem = Get-CimInstance -ClassName Win32_ComputerSystem
if (-not $ComputerSystem.PartOfDomain) {
    try {
        "join domain" | Out-Info
        Add-Computer -DomainName $DomainName -Credential $Credential
    } catch {
        $PSItem | Out-Fatal
    }

    "restart" | Out-Info
    Restart-Computer -Force
} elseif ($DomainName -ne $ComputerSystem.Domain) {
    # should never happen
    "joined wrong domain: $($ComputerSystem.Domain), expected: $DomainName" | Out-Fatal
}

$isRDLSRoleInstalled = (Get-WindowsFeature -Name RDS-Licensing).Installed
if (-not $isRDLSRoleInstalled) {
    try {
        "approve server in active directory" | Out-Info
        Approve-RDLServer -DomainName $DomainName -Credential $Credential

        "install role" | Out-Info
        Install-WindowsFeature -Name RDS-Licensing -IncludeManagementTools

        "activate server" | Out-Info
        Import-Module RemoteDesktopServices
        Set-Item -Path 'RDS:\LicenseServer\Configuration\FirstName'     -Value 'Yantoso'
        Set-Item -Path 'RDS:\LicenseServer\Configuration\LastName'      -Value 'Cloud'
        Set-Item -Path 'RDS:\LicenseServer\Configuration\Company'       -Value 'Yantoso LTD'
        Set-Item -Path 'RDS:\LicenseServer\Configuration\CountryRegion' -Value 'Russia'
        Set-Item -Path 'RDS:\LicenseServer\ActivationStatus' -Value "1" -ConnectionMethod "AUTO" -Reason "5"

        $LicensePack = @{
            AgreementType    = [UInt32]4 # 4 is SPLA
            LicenseCount     = [UInt32]5
            ProductType      = [UInt32]1 # 1 is PerUser
            ProductVersion   = [UInt32]6 # 6 is 2019
            sAgreementNumber = [string]88637253; # very sensitive field
        }
        Invoke-CimMethod -ClassName 'Win32_TSLicenseKeyPack' -MethodName InstallAgreementLicenseKeyPack $LicensePack
    } catch {
        $PSItem | Out-Fatal
    }

    "restart" | Out-Info
    Restart-Computer -Force
} else {
    "deployment complete" | Out-Info
    Unregister-ScheduledTask -TaskName 'deploy' -Confirm:$false
}
