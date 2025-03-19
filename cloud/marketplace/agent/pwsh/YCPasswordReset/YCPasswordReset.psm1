Set-Variable -Name KeyLength -Option Constant -Value 2048

function New-RSAPrivateKey {
    return [System.Security.Cryptography.RSACng]::New($KeyLength)
}

Set-Variable -Name Public -Option Constant -Value $true

function Export-Modulus {
    param(
        [Parameter(Mandatory=$true)]
        [System.Security.Cryptography.RSACng]
        [ValidateNotNull()]
        $RSAPrivateKey
    )

    $PrivateKeyParameters = $RSAPrivateKey.ExportParameters($Public)
    $Modulus = $PrivateKeyParameters.Modulus
    $Base64Modulus = [System.Convert]::ToBase64String($Modulus)

    return $Base64Modulus
}

function Export-Exponent {
    param(
        [Parameter(Mandatory=$true)]
        [System.Security.Cryptography.RSACng]
        [ValidateNotNull()]
        $RSAPrivateKey
    )

    $PrivateKeyParameters = $RSAPrivateKey.ExportParameters($Public)
    $Exponent = $PrivateKeyParameters.Exponent
    $Base64Exponent = [System.Convert]::ToBase64String($Exponent)

    return $Base64Exponent
}

function Get-Unixtime { return ConvertTo-Unixtime -Date (Get-Date) }

function ConvertTo-Unixtime {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateNotNull()]
        [datetime]$Date
    )

    $Epoch = [int64]([double]::Parse((Get-Date -Date ($Date) -UFormat "%s"),[CultureInfo][System.Threading.Thread]::CurrentThread.CurrentCulture))

    return $Epoch
}

function ConvertFrom-EncryptedString {
    param(
        [Parameter(Mandatory=$true)]
        [System.Security.Cryptography.RSACng]
        [ValidateNotNull()]
        $RSAPrivateKey,

        [Parameter(Mandatory=$true)]
        [ValidateNotNullOrEmpty()]
        [String]$EncryptedString
    )

    $EncryptedBytes = [System.Convert]::FromBase64String($EncryptedString)
    $DecryptedBytes = $RSAPrivateKey.Decrypt($EncryptedBytes, [System.Security.Cryptography.RSAEncryptionPadding]::OaepSHA256)
    $DecryptedString = [System.Text.Encoding]::UTF8.GetString($DecryptedBytes)

    return $DecryptedString
}

Set-Variable -Name Schema -Option Constant -Value "v1"

function New-PasswordResetRequest
{
    param(
        [Parameter(Mandatory = $true)]
        [System.Security.Cryptography.RSACng]
        [ValidateNotNull()]
        $RSAPrivateKey,

        [Parameter(Mandatory = $false)]
        [ValidateNotNullOrEmpty()]
        [String]$Username = "Administrator"
    )

    return [ordered]@{
        Username = $Username
        Modulus  = Export-Modulus -RSAPrivateKey $RSAPrivateKey
        Exponent = Export-Exponent -RSAPrivateKey $RSAPrivateKey
        Expires  = Get-Unixtime
        Schema   = $Schema
    }
}

Set-Variable -Name PasswordResetMessageType -Option Constant -Value "UserChangeRequest"

function New-Message {
    param(
        [Parameter(Mandatory=$true)]
        [ValidateNotNull()]
        [PSObject]$Payload
    )

    return [ordered]@{
        Timestamp = Get-Unixtime
        ID        = [GUID]::Newguid().GUID.ToUpper()
        Type      = $PasswordResetMessageType
        Payload   = $Payload
    }
}

Set-Variable -Name ErrNoCli -Option Constant -Value "configured yc cli required to run this cmdlet"

function Test-CliInstalled {
    if (Get-CliVersion) {
        return $true
    }

    return $false
}

function Get-CliVersion {
    $Command = "& yc version"
    $Version = Invoke-Expression $Command
    if ($VerbosePreference) {
        "yc cli version detected: {0}" -f $Version | Write-Verbose
    }

    return $Version
}

function Test-Envelope {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNull()]
        [PSObject]$Obj
    )

    if ($DebugPreference) {
        "Test it is valid envelope with payload: {0}" -f $Obj | Write-Verbose
    }

    $Members = $Obj | Get-Member
    $Properties = $Members | Where-Object MemberType -eq NoteProperty
    $Fields = $Properties | Select-Object -ExpandProperty Name

    foreach ($Field in @("Timestamp", "ID", "Type", "Payload")) {
        if ($Fields -notcontains $Field) {
            return $false
        }
    }

    return $true
}

function Test-UserChangeResponse {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [PSObject]$Obj
    )

    if ($DebugPreference) {
        "Test it is valid user change response: {0}" -f $Obj | Write-Verbose
    }

    $Members = $Obj | Get-Member
    $Properties = $Members | Where-Object MemberType -eq NoteProperty
    $Fields = $Properties | Select-Object -ExpandProperty Name

    foreach ($Field in @("Modulus", "Exponent", "Username", "EncryptedPassword", "Success", "Error")) {
        if ($Fields -notcontains $Field) {
            return $false
        }
    }

    return $true
}

Set-Variable -Name Timeout -Option Constant -Value 60

Set-Variable -Name COMPort -Option Constant -Value 4

Set-Variable -Name PasswordResetResponseType -Option Constant -Value "UserChangeResponse"

function Reset-YCUserPassword {
    param(
        [Parameter(Mandatory = $true,
            ParameterSetName = 'Name',
            HelpMessage = 'Enter compute instance name')]
        [ValidateNotNullOrEmpty()]
        [String]$InstanceName,

        [Parameter(Mandatory = $true,
            ParameterSetName = 'ID',
            HelpMessage = 'Enter compute instance ID')]
        [ValidateNotNullOrEmpty()]
        [String]$InstanceID,

        [Parameter(Mandatory = $false)]
        [ValidateNotNullOrEmpty()]
        [String]$Username = "Administrator"
    )

    $isCli = Test-CliInstalled
    if (-not $isCli) {
        throw "yc cli is not installed or not in PATH, you must properly install it to proceed"
    }

    $PrivateKey = New-RSAPrivateKey
    $PasswordResetRequest = New-PasswordResetRequest -RSAPrivateKey $PrivateKey -Username $Username
    $Message = New-Message -Payload $PasswordResetRequest
    $RequestWrappedInMessage = $Message | ConvertTo-Json
    $TempFilePath = New-TemporaryFile | Select-Object -ExpandProperty FullName

    if ($VerbosePreference) {
        "Write request`n{0}`nIn temporary file: {1}" -f $RequestWrappedInMessage, $TempFilePath | Write-Verbose
    }
    Set-Content -Path $TempFilePath -Value $RequestWrappedInMessage -ErrorAction:Stop

    $Instance = "--name {0}" -f $InstanceName
    if ($PSCmdlet.ParameterSetName -eq 'ID') {
        $Instance = "--id {0}" -f $InstanceID
    }

    $Command = "& yc compute instance update {0} --metadata-from-file windows-users={1} --format json --no-user-output" -f $Instance, $TempFilePath
    if ($VerbosePreference) {
        "Invoke command: '{0}'" -f $Command | Write-Verbose
    }
    try {
        Invoke-Expression $Command -ErrorAction:Stop | Out-Null
    } catch {
        throw $Error
    } finally {
        Remove-Item -Path $TempFilePath -Confirm:$false
    }

    $StopWatchTimer = New-Object -TypeName System.Diagnostics.Stopwatch
    $StopWatchTimer.Start()

    do {
        $Command = "& yc compute instance get-serial-port-output {0} --port {1} --no-user-output" -f $Instance, $COMPort
        if ($VerbosePreference) {
            "Invoke command: '{0}'" -f $Command | Write-Verbose
        }
        [System.Array]$Output = Invoke-Expression $Command -ErrorAction:Stop

        foreach ($Line in $Output) {
            if ($DebugPreference) {
                "Processing line: {0}" -f $Line | Write-Verbose
            }
            if ([String]::IsNullOrEmpty($line)) {
                continue
            }

            try {
                $Envelope = $Line | ConvertFrom-Json -ErrorAction:SilentlyContinue
            } catch {
                continue
            }

            $isEnvelope = Test-Envelope $Envelope -ErrorAction:SilentlyContinue
            if (-not $isEnvelope) {
                continue
            }

            $isCorrectID = $Envelope.ID -eq $Message.ID
            if (-not $isCorrectID) {
                continue
            }

            $isUserChangeResponseType = $Envelope.Type -eq $PasswordResetResponseType
            if (-not $isUserChangeResponseType) {
                continue
            }

            $Response = $Envelope.Payload
            $isUserChangeResponse = Test-UserChangeResponse $Response -ErrorAction:SilentlyContinue
            if (-not $isUserChangeResponse) {
                continue
            }

            if ($VerbosePreference) {
                "Found valid user change response:`n{0}" -f ($Response | ConvertTo-Json) | Write-Verbose
            }
            if ($Response.Success -eq $true) {
                $Params = @{
                    RSAPrivateKey = $PrivateKey
                    EncryptedString = $Response.EncryptedPassword
                }
                $Password = ConvertFrom-EncryptedString @Params

                return $Password
            }
        }

        Start-Sleep -Seconds 1
    } while ($StopWatchTimer.Elapsed.Seconds -lt $Timeout)

    if ($null -ne $Response) {
        throw "error processing password change request: $($Response.Error)"
    }
    throw "timeout processing password change request"
}
