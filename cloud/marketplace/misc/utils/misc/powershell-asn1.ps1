function Get-ASN1NumberBytesToRead
{
    param(
        [System.IO.BinaryReader]$BinaryReader
    )

    $BinaryLength = $BinaryReader.ReadByte()

    # if highest bit is 1 (7th), other bits (6 to 0)
    # show number of bytes, representing length
    # to simplify my life a bit, i assume
    # that there is no more than 4 bytes (UInt32)

    if ($BinaryLength -band 0x80) {
        # get number of bytes representing length
        # removing highest bit
        $NumberOfBytes = ($BinaryLength -shl 1) -shr 1
        $BinaryLength = $BinaryReader.ReadBytes($NumberOfBytes)
    }

    $BytesNeeded = 4 - $BinaryLength.Length
    $Zeros = [Array]::CreateInstance([Byte].FullName, $BytesNeeded)
    $BinaryLength = $Zeros + $BinaryLength

    # endianess, BitConverter uses system values
    if ([System.BitConverter]::IsLittleEndian)
    {
        [System.Array]::Reverse($BinaryLength)
    }

    return [System.BitConverter]::ToInt32($BinaryLength, 0)
}

function ConvertFrom-Asn1Binary {
    param(
        [byte[]]$ByteArray
    )
    
    $MemoryStream = [System.IO.MemoryStream]::new($ByteArray, $false)
    $BinaryReader = [System.IO.BinaryReader]::new($MemoryStream)

    while ($BinaryReader.BaseStream.Position -ne $BinaryReader.BaseStream.Length) {
        $BinaryType = $BinaryReader.ReadByte()
        $BytesToRead = Get-ASN1NumberBytesToRead -BinaryReader $BinaryReader

        switch ($BinaryType) {
            0x03 { # 'BIT_STRING'
                $TypeName = 'BIT_STRING'
                
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)

                $BitsAtLastByteToIgnore = $ValueBytes[0]
                
                if ($BitsAtLastByteToIgnore -band 0x0F) {
                    foreach ($Byte in $ValueBytes[1..$BytesToRead]) {
                        [string]$BinaryString += [System.Convert]::ToString($Byte, 2)
                    }

                    # trimming unused bits
                    $BinaryString = $BinaryString.Remove($BinaryString.Length - $BitsAtLastByteToIgnore)

                    # padding with zeros
                    $BinaryString = $BinaryString.PadLeft($BinaryString.Length + $BitsAtLastByteToIgnore, '0')
                
                    for ($i = 0; $i -lt $BinaryString.Length / 8; $i++)
                    {
                        $Substring = $BinaryString.Substring($i * 8, 8)
                        [byte[]]$RestoredBytes += [System.Convert]::ToByte($Substring, 2)
                    }
                
                    # need to do something with that...
                    $Value = ConvertFrom-Asn1Binary -ByteArray $RestoredBytes
                } else {
                    $Value = ConvertFrom-Asn1Binary -ByteArray $ValueBytes[1..$BytesToRead]
                }
            

                break
            }

            0x01 { # 'BOOLEAN'
                $TypeName = 'BOOLEAN'
                # will always read 1 byte
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = [System.Convert]::ToBoolean($ValueBytes)            

                break
            }

            0x02 { # 'INTEGER'
                $TypeName = 'INTEGER'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = [System.Convert]::ToBase64String($ValueBytes)

                break
            }

            0x05 { # 'NULL'
                $TypeName = 'NULL'
                # will always read 1 byte
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = $null

                break
            }
            0x06 { # 'OID'
                $TypeName = 'OID'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)

                # VLQ
                $Value = ConvertFrom-VLQ -ByteArray $ValueBytes

                break
            }

            0x04 { # 'OCTET_STRING'
                $TypeName = 'OCTET_STRING'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = ConvertFrom-Asn1Binary -ByteArray $ValueBytes

                break
            }

            0x1E { # 'UNICODE_STRING'
                $TypeName = 'UNICODE_STRING'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = [System.Text.Encoding]::Unicode.GetString($ValueBytes)

                break
            }

            0x16 { # 'IA5tring'
                $TypeName = 'IA5tring'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = [System.Text.Encoding]::ASCII.GetString($ValueBytes)

                break
            }

            0x13 { # 'PrintableString'
                $TypeName = 'PrintableString'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = [System.Text.Encoding]::ASCII.GetString($ValueBytes)

                break
            }

            0x0C { # 'UTF8String'
                $TypeName = 'UTF8String'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = [System.Text.Encoding]::UTF8.GetString($ValueBytes)

                break
            }

            0x30 { # 'SEQUENCE'
                $TypeName = 'SEQUENCE'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = ConvertFrom-Asn1Binary -ByteArray $ValueBytes

                break
            }

            0x31 { # 'SET'
                $TypeName = 'SET'
                $ValueBytes = $BinaryReader.ReadBytes($BytesToRead)
                $Value = ConvertFrom-Asn1Binary -ByteArray $ValueBytes

                break
            }

            default { throw 'UNDEFINED TYPE' }
        }
            
        # Type Length Value
        $obj = New-Object -TypeName PSObject
        $obj | Add-Member -MemberType NoteProperty -Name Tag         -Value $BinaryType
        $obj | Add-Member -MemberType NoteProperty -Name TagName     -Value $TypeName
        $obj | Add-Member -MemberType NoteProperty -Name Length      -Value $BytesToRead
        $obj | Add-Member -MemberType NoteProperty -Name Value       -Value $Value
        $obj | Add-Member -MemberType NoteProperty -Name ValueBytes  -Value $ValueBytes

        [array]$Result += $obj
    }

    return $Result
}

# for OID
function ConvertFrom-VLQ {
    param(
        [byte[]]$ByteArray
    )

    $Number = [System.Convert]::ToInt32($ByteArray[0])

    $Value = "$(($Number - $Number % 40 ) / 40).$($Number % 40)"

    for ($i = 1; $i -lt $ByteArray.Length; $i++) {
        if ($ByteArray[$i] -band 0x80) {
            # higher bit is 1 -> VLQ long form
            # 1 in last bit (7th) bit means 
            # that value span to next byte 
            [string]$BitString = ""

            while ($ByteArray[$i] -band 0x80) {
                [string]$BitString += [System.Convert]::ToString($ByteArray[$i] -band 0x7F, 2)
                $i++
            }
            
            $BitString += [System.Convert]::ToString($ByteArray[$i], 2).PadLeft(7, '0')
            $Number = [System.Convert]::ToInt32($BitString, 2)
        } else {
            # less than 128 -> VLQ short form
            $Number = [System.Convert]::ToInt32($ByteArray[$i])         
        }

        $Value += ".$Number"
    }

    return $Value
}

function Remove-Padding {
    param(
        [byte[]]$ByteArray
    )

    if ($ByteArray[0] -eq 0x00) {
        return $ByteArray[1..$ByteArray.Length]
    }
    else {
        return $ByteArray
    }
}

function Remove-PEMDecorations {
    param(
        [string]$String
    )

    return $String `
        -replace '-----BEGIN PUBLIC KEY-----', '' `
        -replace '-----END PUBLIC KEY-----', '' `
        -replace '-----BEGIN PRIVATE KEY-----', '' `
        -replace '-----END PRIVATE KEY-----', '' `
        -replace "`r`n", "" `
        -replace "`n", "" `
        -replace "`r", ""
}

<#
##### EXAMPLE
# ENCRYPT
$json = Get-Content C:\Users\mryzh\Desktop\YC\bb\msft\exchange\terraform-service-account.json | ConvertFrom-Json
$Base64String = Remove-PEMDecorations -String $json.public_key
$ByteArray = [System.Convert]::FromBase64String($Base64String)
$asn = ConvertFrom-Asn1Binary -ByteArray $ByteArray
$PublicKey = $asn.Value[1].Value.Value

$RSAParameters = New-Object -TypeName System.Security.Cryptography.RSAParameters
$RSAParameters.Modulus  = Remove-Padding -ByteArray $PublicKey[0].ValueBytes
$RSAParameters.Exponent = Remove-Padding -ByteArray $PublicKey[1].ValueBytes
$RSA = [System.Security.Cryptography.RSACryptoServiceProvider]::Create($RSAParameters)

$MySecret = "awfully secret string"
$MySecretBytes = [System.Text.Encoding]::UTF8.GetBytes($MySecret)

$EncryptedBytes = $RSA.Encrypt($MySecretBytes, [System.Security.Cryptography.RSAEncryptionPadding]::Pkcs1)

# DECRYPT
$json = Get-Content C:\Users\mryzh\Desktop\YC\bb\msft\exchange\terraform-service-account.json | ConvertFrom-Json
$Base64String = Remove-PEMDecorations -String $json.private_key
$ByteArray = [System.Convert]::FromBase64String($Base64String)
$asn = ConvertFrom-Asn1Binary -ByteArray $ByteArray
$PrivateKey = $asn.Value[-1].Value.Value

$RSAParameters = New-Object -TypeName System.Security.Cryptography.RSAParameters
$RSAParameters.Modulus  = Remove-Padding -ByteArray $PrivateKey[1].ValueBytes
$RSAParameters.Exponent = Remove-Padding -ByteArray $PrivateKey[2].ValueBytes
$RSAParameters.D        = Remove-Padding -ByteArray $PrivateKey[3].ValueBytes
$RSAParameters.P        = Remove-Padding -ByteArray $PrivateKey[4].ValueBytes
$RSAParameters.Q        = Remove-Padding -ByteArray $PrivateKey[5].ValueBytes
$RSAParameters.DP       = Remove-Padding -ByteArray $PrivateKey[6].ValueBytes
$RSAParameters.DQ       = Remove-Padding -ByteArray $PrivateKey[7].ValueBytes
$RSAParameters.InverseQ = Remove-Padding -ByteArray $PrivateKey[8].ValueBytes
$RSA = [System.Security.Cryptography.RSACryptoServiceProvider]::Create($RSAParameters)

$DecryptedBytes = $RSA.Decrypt($EncryptedBytes, [System.Security.Cryptography.RSAEncryptionPadding]::Pkcs1)

[System.Text.Encoding]::UTF8.GetString($DecryptedBytes)
#>
