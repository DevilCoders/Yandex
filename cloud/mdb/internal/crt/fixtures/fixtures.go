// Package fixtures contains test certificate
package fixtures

import "time"

// Parts of test certificate
const (
	PrivateKey = `-----BEGIN PRIVATE KEY-----
MIHcAgEBBEIAcwi4G3qIQgeFEtPx44Oc9SMN6UiJctQ/TbUBh6K7TDWErtvObUSU
w7VeQEkFSf9M6FCU0yNrOqWb4W3yQTOKrWOgBwYFK4EEACOhgYkDgYYABADKUn5/
S5urCWfX07yGPy9hziKb1cO00VQYZT+5uoK+5XBqcHTXBB4R42vdqw142E1c5pX0
yt3vVc1mxpEasdK0tgFfBKaoh9Eywz+PiU9c7L3pjhIpGmLoO3TJfOd9MsLAswlH
eppq2R0KwCJkCq9Kzqle6rSOyHBkKZ59zSDHHWu3ZA==
-----END PRIVATE KEY-----
`

	FirstCert = `-----BEGIN CERTIFICATE-----
MIIB0DCCATGgAwIBAgIBATAKBggqhkjOPQQDBDASMRAwDgYDVQQKEwdBY21lIENv
MB4XDTE5MDUzMDE4MTMwOFoXDTIxMTExNTE4MTMwOFowEjEQMA4GA1UEChMHQWNt
ZSBDbzCBmzAQBgcqhkjOPQIBBgUrgQQAIwOBhgAEAMpSfn9Lm6sJZ9fTvIY/L2HO
IpvVw7TRVBhlP7m6gr7lcGpwdNcEHhHja92rDXjYTVzmlfTK3e9VzWbGkRqx0rS2
AV8EpqiH0TLDP4+JT1zsvemOEikaYug7dMl8530ywsCzCUd6mmrZHQrAImQKr0rO
qV7qtI7IcGQpnn3NIMcda7dkozUwMzAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAww
CgYIKwYBBQUHAwEwDAYDVR0TAQH/BAIwADAKBggqhkjOPQQDBAOBjAAwgYgCQgHy
0BKzmWeVjx9QJMnzCrGdgd7nFQSBDh1QsO326bG9NW7lYKexgyRk1gVLEHIhh543
u1u0z1Du054cGL24xdh9mwJCAIJLBlkzYWrlP3C9HrTn+fiZJy4T+2Otyj//1viP
/+GzppTJoZZtAuvQ40bJ2UOyZspMeVlGhiVOzd0LROY9R7/F
-----END CERTIFICATE-----
`
	SecondCert = `-----BEGIN CERTIFICATE-----
MIIB0DCCATGgAwIBAgIBATAKBggqhkjOPQQDBDASMRAwDgYDVQQKEwdBY21lIENv
MB4XDTE5MDUzMDE4MTMwOFoXDTIxMDgwNzE4MTMwOFowEjEQMA4GA1UEChMHQWNt
ZSBDbzCBmzAQBgcqhkjOPQIBBgUrgQQAIwOBhgAEAMpSfn9Lm6sJZ9fTvIY/L2HO
IpvVw7TRVBhlP7m6gr7lcGpwdNcEHhHja92rDXjYTVzmlfTK3e9VzWbGkRqx0rS2
AV8EpqiH0TLDP4+JT1zsvemOEikaYug7dMl8530ywsCzCUd6mmrZHQrAImQKr0rO
qV7qtI7IcGQpnn3NIMcda7dkozUwMzAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAww
CgYIKwYBBQUHAwEwDAYDVR0TAQH/BAIwADAKBggqhkjOPQQDBAOBjAAwgYgCQgH5
OxJw6sjs/lk9lpVH6AdHNsRvMuifm9qkrRKJ+m0p9wp8+xyx6b5QSVD+R7eLqKpF
7oK+IWxNi2fVwn0P2WWKNgJCAceDrsAVS6Svjk2KyYxoEIF1AGYo5BW9Syrss0Mt
JmsQcgQXvyGK6+HUq4espYaBE3aTt7MREBPZcoH2cDj6ns3T
-----END CERTIFICATE-----
`
	ThirdCert = `-----BEGIN CERTIFICATE-----
MIIB0DCCATGgAwIBAgIBATAKBggqhkjOPQQDBDASMRAwDgYDVQQKEwdBY21lIENv
MB4XDTE5MDUzMDE4MTMwOFoXDTIxMDgwNzE4MTMwOFowEjEQMA4GA1UEChMHQWNt
ZSBDbzCBmzAQBgcqhkjOPQIBBgUrgQQAIwOBhgAEAMpSfn9Lm6sJZ9fTvIY/L2HO
IpvVw7TRVBhlP7m6gr7lcGpwdNcEHhHja92rDXjYTVzmlfTK3e9VzWbGkRqx0rS2
AV8EpqiH0TLDP4+JT1zsvemOEikaYug7dMl8530ywsCzCUd6mmrZHQrAImQKr0rO
qV7qtI7IcGQpnn3NIMcda7dkozUwMzAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0lBAww
CgYIKwYBBQUHAwEwDAYDVR0TAQH/BAIwADAKBggqhkjOPQQDBAOBjAAwgYgCQgEP
5tkClBZstR0AtAyYN75fosLJTDXyvE4NwFNxLSIxIiwyJXt/4zWyMCIiO4bM8+Tg
rxFnTIuc4/xvZreAAeO5WQJCAMJDTM1cjafg5UmAeWcPQjSEA56T2d14E8bUknum
Ck/VDy9lyGo7Vy8WAeM4nI67HwlyRhJoau+NdDiilt2qoTcp
-----END CERTIFICATE-----
`

	FullPem  = PrivateKey + "\n" + FirstCert + "\n" + SecondCert + ThirdCert
	FullCert = FirstCert + SecondCert + ThirdCert
)

// CertEndDate is certificate's expiration date
var CertEndDate, _ = time.Parse(time.RFC3339, "2021-08-07T18:13:08Z")
