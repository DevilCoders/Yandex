function Get-S3Date {
  return (Get-Date).ToUniversalTime().ToString("ddd, dd MMM yyyy HH:mm:ss 'GMT'")
}

function Get-S3Signature {
  param(
    $s3Secret,
    $stringToSign
  )

  $HMACSHA1 = [System.Security.Cryptography.KeyedHashAlgorithm]::Create("HMACSHA1")
  $HMACSHA1.Key = [System.Text.Encoding]::UTF8.Getbytes($s3Secret)
  $Signature = [Convert]::Tobase64String($HMACSHA1.ComputeHash([System.Text.Encoding]::UTF8.Getbytes($stringToSign)))

  return [Convert]::Tobase64String($HMACSHA1.ComputeHash([System.Text.Encoding]::UTF8.Getbytes($stringToSign)))
}

function Download-S3Object {
  param(
    $object,
    $bucket,
    $s3Key = ($env:AWS_ACCESS_KEY_ID),
    $s3Secret = ($env:AWS_SECRET_ACCESS_KEY),
    $dir = $PWD,
    $contentType = "application/x-compressed-zip"
  )

  # its 2020, but supported in 6.0+
  #[Net.ServicePointManager]::SecurityProtocol +='tls12'

  # workaround
  $pp = $ProgressPreference
  $ProgressPreference = 'SilentlyContinue'

  $resource     = "/$bucket/$object"
  $date         = Get-S3Date
  $signedString = Get-S3Signature -s3Secret $s3Secret -stringToSign "GET`n`n$contentType`n$date`n$resource"

  $params = @{
    Header = @{
      "Host"          = "storage.yandexcloud.net"
      "Date"          = $date
      "Content-Type"  = $contentType
      "Authorization" = "AWS $($s3Key):$($signedString)"
    }
    Method = "Get"
    Uri = ("https://storage.yandexcloud.net" + "$resource")
    OutFile = Join-Path $dir $object
  }

  #Invoke-RestMethod @params

  $done = $false
  $retry = 1
  $max_retries = 5

  do {
    if ($retry -gt $max_retries ) {
      throw "No retries left"
    }

    try {
      Invoke-WebRequest @params -ea:Stop
      $done = $true
    } catch [System.IO.IOException] {
      Write-Output "Got retryable exception, sleep for $(1 * $retry)s, message: $($_.Exception.Message)"
      Start-Sleep -Seconds (1 * $retry)
    } finally {
      $retry++
    }
  } while (-not $done)

  # /workaround
  $ProgressPreference = $pp
}

function Upload-S3Object {
  param(
    $object,
    $bucket,
    $filePath,
    $s3Key = ($env:AWS_ACCESS_KEY_ID),
    $s3Secret = ($env:AWS_SECRET_ACCESS_KEY),
    $contentType = "application/x-compressed-zip"
  )

  # workaround
  $pp = $ProgressPreference
  $ProgressPreference = 'SilentlyContinue'

  $resource     = "/$bucket/$object"
  $date         = Get-S3Date
  $signedString = Get-S3Signature -s3Secret $s3Secret -stringToSign "PUT`n`n$contentType`n$date`n$resource"

  $params = @{
    Header = @{
      "Host"          = "storage.yandexcloud.net"
      "Date"          = $date
      "Content-Type"  = $contentType
      "Authorization" = "AWS $($s3Key):$($signedString)"
    }
    Method = "Put"
    Uri = ("https://storage.yandexcloud.net" + "$resource")
    InFile = $filePath
  }
  Invoke-WebRequest @params -ea:Stop
}