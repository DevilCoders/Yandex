#!/bin/bash

set -ex

# Enable this script to be run on each boot.
sed -i "s/ - scripts-user/ - [scripts-user, always]/g" /etc/cloud/cloud.cfg


# Install GPN Internal Root CA.

TECHPARK_CRT=$'-----BEGIN CERTIFICATE-----
MIIFBzCCAu+gAwIBAgIQf3ZapO99jrtCJbMPZ6ulmTANBgkqhkiG9w0BAQ0FADAW
MRQwEgYDVQQDEwtUZWNocGFyayBDQTAeFw0xOTExMTgxNTE3NTNaFw0zNDExMTgx
NTI3NTJaMBYxFDASBgNVBAMTC1RlY2hwYXJrIENBMIICIjANBgkqhkiG9w0BAQEF
AAOCAg8AMIICCgKCAgEAu3l0C48YZ4qA7M7ObJJR60OFGdzhoK6qNYBq52j38szj
SGmIXPlUzKslelniqppou5JG+p0T9YNZpyhxyLxdpNDYLSFpt01z4sdpnCdZLIzf
F6/A1UA5q073JabOLk856QqXxFUd/ctY7H1L1kNSZbLoxOMGXQCEqGIIBuJQYHQC
np+KTS8Vsv1OXC+bpwgv7SrBYNBD3o//jHrtF9IPkjJr/TgDDzJ9tPwOFtRY6CTg
JrzyOOp92mds2s7SE+ab6CwzKgeWpzhD+42whskmoI8ZT4MrmC8zxWtUlriXRPDy
OA/NZcD0G5jpPX9rR/M2OxO9Z8bJB5VWzo6TAWdRYcM7jhkRHoVnsM3z3HIN2qzh
+u2ZGTLUsRJTidY2l5NJrebHjMyA1ka/XxFmhKo8J1qiNiMjgZkrLjYAxSGk/Fq1
rbNOZUFEnzs6NI2gcWZmS4cDQfvlJmPWa9fYiEDLrK8DJzRnQuKd46K9YoL2ZZIE
YtdJCXD402BLm5Q3MnMyip29rzZfvVx2ZniR1n3fC75kDIVhNSWUfpfWqvzdzs5o
sFp+bqGdhLa76KGt5CYTV5O96s0cQLght+kzVxkYpm4uoC0un8Y6d8QL+U9xjBDP
ngldSuesGw/TQnHsEcG2C63OZ8x0G9hnJvtzktEK1WGw53hLqFBTGtsmegvvwzEC
AwEAAaNRME8wCwYDVR0PBAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYE
FMj1hqV7fFBdnF4N+OhgM/fT+iaKMBAGCSsGAQQBgjcVAQQDAgEAMA0GCSqGSIb3
DQEBDQUAA4ICAQBBTmN6vApbZADtGHomZqFimtnl0nCGE9HSg1IzFuS6DtP1sfc6
qOQK3bDRZ+Me3y4u0p1eKz17Ha8WEc0RMvtNw0W53kvNWSwNRGHT8QM73Dilib30
+dcIqWIYW+sQfxn5jPbdNW4S7wgGTAasuCvZy/rxagT2nMgEvrBICjq9Emi856Bp
LFD25Eu2sRgDCuEX2BrNptntJYpH/nYvK6ZTiMryPbdHxuh+z7HIy/II90Ek24vc
KyinH90ltc/p0AyM6iqJ7EaqiiZF9pLMFgUaFHRxMzrSLVrjW53/tYeA6HG9gawO
wS5A/nyngmDp5o9eQgR0VdSD1UxUMdVpb/7sIVMqaVrHfNOWJ8PvFzRnnaP1N97m
JMUanyLPYBY5qSKe0vf8ThTtjDNbMXb8Kk1wooxBKGDkFbr9XBfnCQ+7qZ/6bylW
SQdvyDe2cE6oSmdaXXvU4SaiPswDOm15zXd5OdhEbAvI/Pj5kbEFP5MckXAQqcxE
Z398DvH7/cfoN7b9nviknMfFQBminbYBV1UHwn3EkTEzE21lYe16QwNEGPw296pD
7vitHLXlo3NuL6pu4dRYoIgWxF5VVwO7PAx6YT2EsX8gGC3YuaKlJjZI/A0jG/iM
mal5+6axzjZUswjMGufG9UCnl0Eh+Mlk4grBGNKLlN3RsK2D9VIi7wFt+Q==
-----END CERTIFICATE-----'

GPN_INTERNAL_CRT=$'-----BEGIN CERTIFICATE-----
MIIDujCCAqICCQDpeZBSW2N5mTANBgkqhkiG9w0BAQsFADCBnjELMAkGA1UEBhMC
UlUxGzAZBgNVBAgMElJ1c3NpYW4gRmVkZXJhdGlvbjEPMA0GA1UEBwwGTW9zY293
MQwwCgYDVQQKDANHUE4xEjAQBgNVBAsMCUdQTi5DbG91ZDEaMBgGA1UEAwwRR1BO
SW50ZXJuYWxSb290Q0ExIzAhBgkqhkiG9w0BCQEWFGluZnJhQHlhbmRleC10ZWFt
LnJ1MB4XDTIwMDYwMzE2MDcyMloXDTMwMDYwMTE2MDcyMlowgZ4xCzAJBgNVBAYT
AlJVMRswGQYDVQQIDBJSdXNzaWFuIEZlZGVyYXRpb24xDzANBgNVBAcMBk1vc2Nv
dzEMMAoGA1UECgwDR1BOMRIwEAYDVQQLDAlHUE4uQ2xvdWQxGjAYBgNVBAMMEUdQ
TkludGVybmFsUm9vdENBMSMwIQYJKoZIhvcNAQkBFhRpbmZyYUB5YW5kZXgtdGVh
bS5ydTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAOxIyTAe4P3zGXuI
xJp3NwIQGKmErvJgHAbFYU9v5sueufWvPvyt7HU/bjROJsMXK7sWwPq3l2rup5OW
jTtbBWsmX3eHeLxffr+KYkKb8tSGWDFMs5jx9xp7C0IPj5aMEZiTumVn90u5rUwA
j3+csNZvKqruQn+/E9DtUseYq6rtW0YCS+uQzEYvzVGkWhsLPwa1VbuKOoaF+Pfb
RYXD/Iobq8yz2Wr87oQVQuACq9L0wv5THCRReTZnkDP13iGZWIpPz9FpNI+JDCR3
3OrqqtXzNogQnqXj/zFecL/ymgXOl28iSmb/OKZpBs2DUXSk0YZngHAgJ+6XeYI3
x2wSUcUCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEA30Rywe2bW5RqTnf/Yexj1PmM
PHj6ZpuR4pXT+CX8Sfmsafd5lM115T5pzLfimum6rDylQ3voBY+vt3QIjWR/j18E
0fkszNS5HSkz8gx6J6OVGNcupUSaboMmaoq9zAxi/SWkPyT00TD1xIKf0g29PXPo
mfJic6a0f1rVuzUqJBU5nHoA+5geqgwDC0jf/Zq9nWcIyLE1VBKK5RzSPqRzOP/J
B1Xv7lCZv6DdS4C/RlHHIKki6Bcez8BprM/fK2n4NcOtIWqRBywGKo2t0Uc+DnIf
Bgmg1oEQl5wDwiiloPB8as2s36apjfHgGKLM+Gom1vBMNiE2Nmej48gkM92s1A==
-----END CERTIFICATE-----'

JAVA_CA_CERTS="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"

echo "$GPN_INTERNAL_CRT" > /usr/local/share/ca-certificates/GPNInternalRootCA.crt
echo "$TECHPARK_CRT" > /usr/local/share/ca-certificates/TechparkCA.crt
update-ca-certificates

mkdir -p /etc/kms
echo $JAVA_CA_CERTS | base64 -d > /etc/kms/gpn-ca-certs


# Use NTP proxy.
sed -i "/^NTP=/d" /etc/systemd/timesyncd.conf
echo "NTP=ntp.proxy.gpn.yandexcloud.net" >> /etc/systemd/timesyncd.conf
systemctl restart systemd-timesyncd


# Use SelfDNS proxy.
sed -i "/^service =/d" /etc/yandex/selfdns-client/default.conf.orig
echo "service = https://selfdns-api.proxy.gpn.yandexcloud.net" >> /etc/yandex/selfdns-client/default.conf.orig
systemctl restart selfdns-client
# echo "service = https://selfdns-api.cloud.yandex.net" >> /etc/yandex/selfdns-client/default.conf
# systemctl restart selfdns-client


# Use /etc/hosts proxy for cr.yandex/storage.yandecloud.net
sed -i "/cr.yandex/d" /etc/hosts
echo "2a0d:d6c0:2:201::a2 cr.yandex" >> /etc/hosts
sed -i "/storage.yandexcloud.net/d" /etc/hosts
echo "2a0d:d6c0:2:201::a2 storage.yandexcloud.net" >> /etc/hosts


# Use /etc/hosts proxy for *logbroker.yandex.net
sed -i "/logbroker.yandex.net/d" /etc/hosts
echo "2a0d:d6c0:2:200::2fc lbkx.logbroker.yandex.net myt0-0481.lbkx.logbroker.yandex.net vla0-5383.lbkx.logbroker.yandex.net sas0-6624.lbkx.logbroker.yandex.net myt0-0497.lbkx.logbroker.yandex.net vla0-5382.lbkx.logbroker.yandex.net sas0-6572.lbkx.logbroker.yandex.net vla0-5359.lbkx.logbroker.yandex.net myt0-0480.lbkx.logbroker.yandex.net myt0-0483.lbkx.logbroker.yandex.net myt0-0477.lbkx.logbroker.yandex.net vla0-5367.lbkx.logbroker.yandex.net vla0-5368.lbkx.logbroker.yandex.net sas0-6704.lbkx.logbroker.yandex.net myt0-0476.lbkx.logbroker.yandex.net sas0-6713.lbkx.logbroker.yandex.net myt0-0472.lbkx.logbroker.yandex.net vla0-5381.lbkx.logbroker.yandex.net vla0-5362.lbkx.logbroker.yandex.net sas0-6574.lbkx.logbroker.yandex.net vla0-5384.lbkx.logbroker.yandex.net sas0-6708.lbkx.logbroker.yandex.net myt0-0474.lbkx.logbroker.yandex.net sas0-6711.lbkx.logbroker.yandex.net sas0-6573.lbkx.logbroker.yandex.net myt0-0488.lbkx.logbroker.yandex.net sas0-6712.lbkx.logbroker.yandex.net vla0-5378.lbkx.logbroker.yandex.net myt0-0495.lbkx.logbroker.yandex.net myt0-0489.lbkx.logbroker.yandex.net vla0-5386.lbkx.logbroker.yandex.net vla0-5377.lbkx.logbroker.yandex.net vla0-5363.lbkx.logbroker.yandex.net sas0-6578.lbkx.logbroker.yandex.net vla0-5360.lbkx.logbroker.yandex.net myt0-0493.lbkx.logbroker.yandex.net sas0-6706.lbkx.logbroker.yandex.net sas0-6576.lbkx.logbroker.yandex.net myt0-0494.lbkx.logbroker.yandex.net sas0-6710.lbkx.logbroker.yandex.net vla0-5380.lbkx.logbroker.yandex.net sas0-6707.lbkx.logbroker.yandex.net myt0-0484.lbkx.logbroker.yandex.net vla0-5369.lbkx.logbroker.yandex.net sas0-6709.lbkx.logbroker.yandex.net sas0-6571.lbkx.logbroker.yandex.net myt0-0473.lbkx.logbroker.yandex.net sas0-6715.lbkx.logbroker.yandex.net myt0-0498.lbkx.logbroker.yandex.net vla0-5376.lbkx.logbroker.yandex.net sas0-6714.lbkx.logbroker.yandex.net vla0-5385.lbkx.logbroker.yandex.net vla0-5357.lbkx.logbroker.yandex.net vla0-5375.lbkx.logbroker.yandex.net sas0-6575.lbkx.logbroker.yandex.net sas0-6716.lbkx.logbroker.yandex.net sas0-6703.lbkx.logbroker.yandex.net myt0-0485.lbkx.logbroker.yandex.net myt0-0478.lbkx.logbroker.yandex.net vla0-5364.lbkx.logbroker.yandex.net sas0-6705.lbkx.logbroker.yandex.net vla0-5358.lbkx.logbroker.yandex.net myt0-0475.lbkx.logbroker.yandex.net myt0-0486.lbkx.logbroker.yandex.net myt0-0482.lbkx.logbroker.yandex.net myt0-0492.lbkx.logbroker.yandex.net sas0-6577.lbkx.logbroker.yandex.net vla0-5361.lbkx.logbroker.yandex.net vla0-5379.lbkx.logbroker.yandex.net myt0-0487.lbkx.logbroker.yandex.net vla0-5387.lbkx.logbroker.yandex.net myt0-0496.lbkx.logbroker.yandex.net sas0-6702.lbkx.logbroker.yandex.net" >> /etc/hosts
