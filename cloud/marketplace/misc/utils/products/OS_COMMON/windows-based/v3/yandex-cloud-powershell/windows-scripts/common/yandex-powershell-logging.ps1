# ToDo:
#       * add TZ via (Get-TimeZone).BaseUtcOffset.Hours


$__SerialPort = 'COM1'
$__SerialPorts = [System.IO.Ports.SerialPort]::getportnames()
$__isSerialOutputAvailible = $__SerialPorts -contains $__SerialPort


# l8r we can use 'kms service' to encrypt secrets, used on test runs
# need to debug this part, works strange sometimes, but mostly works :)
$__TELEGRAM_BOT_TOKEN = $ENV:__TELEGRAM_BOT_TOKEN
$__TELEGRAM_CHAT = $ENV:__TELEGRAM_CHAT


# you can use it in functions for pretty print
# {"timestamp":"9/2/2020 3:11:55 PM","type":"info","msg":"Test-NLA::NLA setting is 1"}
# ^^^ 'Test-NLA' is a function
function Get-FunctionName {
  param(
    [int]$StackNumber = 1
  )
  
  #dirty but works
  return (
    (
      [string]$(Get-PSCallStack)[$StackNumber].FunctionName) `
        -replace "`\u003c", '' `
        -replace "`\u003e", ''
    )
}

function Write-Telegram ($Message) {
  if ($__TELEGRAM_BOT_TOKEN -and $__TELEGRAM_CHAT) {
    $REQUEST = @{
      Method = "POST"
      Uri = "https://api.telegram.org/bot$__TELEGRAM_BOT_TOKEN/sendMessage"
      Body = @{
        "chat_id" = $__TELEGRAM_CHAT
        "text" = "$Message"
      }
    }
    #irm @REQUEST -ea:SilentlyContinue | Out-Null
  }
}

function Write-COM ($Message) {
  if ($__isSerialOutputAvailible) {
    $SerialPort = New-Object System.IO.Ports.SerialPort($__SerialPort)

    $retry = 5
    while ( (-not $SerialPort.IsOpen) -and ($retry -gt 0) ) {
      try {
        $SerialPort.Open()
      } catch {
        $j = Create-JsonMessage -Message "Error openning $__SerialPort port, retry after 5s..." -type "error"
        Write-Host $j
        Start-Sleep -Seconds 5
      }      
    }

    $SerialPort.Write("`r`n$j")
    $SerialPort.Close()
  }
}

function Create-JsonMessage ($message, $type) {
  $m = [pscustomobject][ordered]@{
    timestamp = (Get-Date).ToString();
    type = $type;
    msg = $message;
  }

  return ($m | ConvertTo-Json -Compress)
}

filter Out-InfoMessage {
  $j = Create-JsonMessage -Message $_ -type "info"
  
  Write-Host $j
  Write-COM -Message $j
  #Write-Telegram -Message $j
}

filter Out-ErrorMessage {
  $j = Create-JsonMessage -Message $_ -type "error"

  Write-Error $j
  Write-COM -Message $j
  #Write-Telegram -Message $j
}
