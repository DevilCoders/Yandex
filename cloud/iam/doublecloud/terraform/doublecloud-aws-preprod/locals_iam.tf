locals {
  access_service_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA+MY5Gzf1+KyCy7a6E79X
v5XrXugVx+uEjs/z9RgMvchxnuO4Eer9pDg/u7hBcmiq35D4qGSmyrDoATHCnuFs
ou/W9/p8JsLYPzUaJTOdkBMADBDDa6HB+ovWrmDvcJlFMQuLz6f51kTGMsgjW8Aj
o4HMT3YF9/z9QC0SgUkPcmAldQ8h9vvwgE4TaZ8hLk3+n4QdtuXL7npzDdLfHIfT
k0S8EkyZOObTkY2wymw/nnFGHinEApSj8mVsmjxhknoUQfMGgSlsjZZkBjvXqLS/
cjM3RoDid+kFVG7OsDUX49PbtR32zVD6t/PUQCHdX+IKLnt41ohJgDr09K02SEVK
G6DD8N2DH7cOkopy9PiGcrlzHj+9WeS1vQ9CCf9eN5d6exMNdiXDD3nSDIlwHCkK
HvIvDVWnFad6X1znBZ0kQbjKwVCBnPGpbbCdKgxFjpGeV2mSyYkt+Z44CkgmWFE2
j2u5cZmhwSDgYYIkxbvb3q+QPd6hE3RWsGclTi3aTnv9c2JOT9siuTq/L7hZRFt7
f7c9hNVvLMHABX7iR982Iid6iZab1nkTex0KBHmW8KQcenxnB6RFwPNl/emGPEqK
g6FRE67yjqNKbjcC/XxVM8wITC93BuHm0clX689+2DfVie4LvzLJxcuV6Vop6tCv
LmlcQMc+hk8yLMu1+OKkZv8CAwEAAQ==
-----END PUBLIC KEY-----
EOF

  iam_control_plane_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA2U3b3i36ESFnsoFia+KA
gYV07Dz1eS/gKzColdOnwmNG2KP7gig7EKobxZhIfnxuOav2iX2WYR+yyJOH3+hI
WXNo9ayuEE49YVkkdB6inkoZLObj2tInKr6PoNaRuPI1kFdu6mMrIxkbirJiUYAW
Vxbu4Ln3RilQxN/cMTZDsptqAqG9HdYbekfysPlsW4FmZBey+Y2eWGzcbDQveJBg
yDNXZXpRrPCAhfxvNMCK2Ezmrq56xz0nl3kJ86HBzMhShRaEdZEzGMH+0m/Uadv0
Y56IWjRPp2qidP3BPFqPwIM9bi/ql9+12LkeRfoJaOntQhCAydjnX4TaxbssPEVo
a1/miRasAJJMKWsMIJLpPJEzZ9gCCII2A7JEFsCg97lk+oMWLstKRzZ715gYS/Ck
W72tQOYB+Sv+n4+S2hn/6APcvWptwWHYMGTHpW6DqCX7XMk/3kpbtlq9kLIr5Rdp
niFmbb/eCaJaVeNc4U4tn3GtZF8XEXHbhzuKJN/wiGtLzorvfa91AW1IY461WncF
VJ2PIH/5VGk6ZN4NrpeWMd1op2tqWWVXIeXtc2nVOEK3sbuP0ibBkm78sH5XinNc
4quBxXnBQ2oIKHZwqPXHgD4CFiSuMx15Vl+J+AanX0jKdBC7iRl94XN+PBxC3SBC
st2XXeJ6RYFGxWurYcYpMW8CAwEAAQ==
-----END PUBLIC KEY-----
EOF

  mfa_service_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA1F0llRb897VUvR04GGNC
HbbPFGluQs1DLhnDPKbCsV9QFG0CafW48sH5tN4kkCdW5O82f7V0J1BqgaV2AcZo
ef3fkmMeVprHtz/3wVpV5zsagxs4RjIR+XDi+QC+3qM0G5BJJD8ibVSuxWKXY8+o
PKI4m5jyUuEagS4DZbusw7rzxAsKNHwGWSlBBKEZwO99ZYT4i7wWUzke8XUSGPrC
4m21mopjFp4dQTb8qtAx6lW3k+XlzpOD5OAyEx2eKQhruxxtT1g1cgptOP70lGM8
uzSNTRUV06hj398eZmHoqScin9SXUNpl0tnAieHdiLOjnXmiGnmpZx6YgNUv5OO/
F3qFSPw5df6MezBt3Dsuye3kejEJ6rx+AJgvBWnqwGOj1DBpSMAiiDRo/qIXTPbM
xR0xfIe/qlrIimXQe2tV7eQ6SJOuXnUtExvPopB45wi4MTrezpjykXFXV0gOQZrl
TdsK+uYtXQGXr/BNtB4+54eBU1KK2qq4dlqnqM0EN0bnETXFKVQTpXWGKqxqoHSA
WI3pVXMDRqc/3BlGJbTthZVik/SVvuEDKUQ7E/enzZ9XGZHlm+SLXxTdBdCj4hrY
IWMjwYvz9KnQ1JIYWLX2thYb5pCHXsnCaQbjx/oVvXDhDIZKWbahdAgXnGCUctp0
aoy+ikxT/bKN12WjXEz9/WsCAwEAAQ==
-----END PUBLIC KEY-----
EOF

  openid_server_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA7oX/oebum23f/H+IWBL3
ti7e32cKS+8i7eSPgHoNULwduY2PrrTxkOq4TG0Wt8XPVZwGHLl5U2uKq4SL5vTp
8G8ZQ7HY8EA0dEGP94OzQdc4xJeSjDGErWGdG3bKGIDBOhul8x44pdrXZkYIp/hS
7kY9jdRcFyh0sEX1RvGEG2a8VhyGbsJIzg/svI13Bi1xhZI054Plp2G1KYHnkOWp
rkvHS2ypMi+4335rKlbt59XgG0gnQWcULkc4pIOEDTmnBi1QHhf8PQvU/bLNIq7o
9sEDZCOYTUulcICTRiGj6K8wjkSQDWG0G01p7K7JWIJaLBDwWnFJSQ7W0qFfRveQ
ztUfksQ77GNnDja607CYW3gCq2pFi0wdGFDAeDoOGdxDn9z46dxPTe+fn6jxpG07
qWe09N2mdjtTtNg5kE7wxI5WPnpAr17RIhnFC9DU9PHNkDkr4HXvdx06oknKZH8F
YzxZtUV8csyWJLadgfXoYaKgjbt5mZjXPZepoDMcdoIweljqO32RzajZktJNSqmQ
azvlNSHyyjXs1Tn8SXbd/fi6unbwDUueGRBtrig9bgyESsI4ktvuwH+SIncC9YKv
A+Gk6m2ZojntgDyT9tBi8UG4767Sgv3NVoX4rmILrQ06lxiZSQ40NcqnLN9WJRKG
d9shpO9zpglThk54pWgJv9cCAwEAAQ==
-----END PUBLIC KEY-----
EOF

  org_service_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA9IRGY0L2C+wSl8zRmpdw
ntHfmzaHcrfu1h2ovVKEldodldGlGvGkxDSAWzntU5ka8ZmnooXVuIr5/+gY9qpC
jDi82UHUtPsaGRZkp1TFOnpjR9GoNEBVdCcTfaTQHcNEdnP8l4NaX5bfR9Ix/ktW
UJ/di+MANWr0hxMLhII3Pvy/84hLTIIS7nCw7JnbZNBOpYjN8QyoIeL4TMz/emw9
gvi4yDS2H/EbWIy/NpJcCECPAfH75OuMK+fnGpVPgtIYaHWcGDjIdEdEWFKF0mcP
YmSZekeod/oo3dEr8hI9z4w/4Hiy6IOlKVe7sSjvmY+xIo0Qz6ZK1sU6wb77ILvF
Iw8w7jp8IbFGzY0Jem719oqYZ3mMjawGxthxhWJx0Uy1nntakkBMOCNrj4xGnh5E
wHMoFTbaNgsPG485uH4iT1yXzSZBadjEkKf+4HRzZkaW3QcaBCia/x/tn001R5l8
yZ93k0q4uFXmfVDaILY7lVlB6i6/xGGmBd1VtaoU7C7SRb4pBKso4j92Jq4BpRd6
/yxeuoVwviXOOJBtjHXaLss/HhQz6+Mh75shnbSLvZDL4+jYaCUWME+VqhqUzDDH
zW+ccm/lHbBAYY/frD/f/oP5IqwKAM24eyt4GkV26chT/u250Cy+7W/pI++qk143
4unyFr89IMufytt8x7HUmkECAwEAAQ==
-----END PUBLIC KEY-----
EOF

  reaper_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAvTUbC/K2eF0bnjrtHBsh
zN+O63Si0rR0s5WviZrgs4LZw7Vg1LRiPZVhHoP22xhvxbPo/d+0X4+JzWmE3hf4
AfSxjJHK/OEBSUD+hT2Vqk2+ConzIK6QTOR4im8T879Lpz9ZtE9t+x1qpZB7UgRT
eV2MOnRoL25WkgfQ017dE3AOk+caQdWfAbUzAw2klX1uTHf31aNOyfQ5ObKCdVoj
7V/JA672uUiujjgH8yk9c6ehG0xj4f71n1Ba0673qzPkH/1fmMPuBzPINLWpuiRT
13SS6bQXYNw5rV7LfQsSZfxgxePv0XHwrv+qWBTng5y4xtqff64Qha/OUeu14dL/
7gGaN2m6z4Bcip8g/gXYJ22AOhK5AJfVZqNKzOqJ0DVXt5l5izPq7KwRWnVSHTrG
bINJTXPwtpqlXYtK7BFe5FYxYHmuxk5gI7ebKSCCKkjwIr7c29SUYPaOOG8s9zcL
JKfi0NMA4UPUmAdeuUm1qMfbPcW3k6i27DM1RKGRQ/lMvxoXXQnTvU3XvxLjDNoe
dg0zIkKQW59Lrea0O6ohdr/gXxF9wJWsNy0BEq9MOSEZ/H/+YMoNoTvxM5yJ2Fhr
Z4r7cEOunEaJgYC9vv9lZlQ6X9gzNWqROyOjzlJDKbRpJVD8IrkNi8uaM/p/6y9z
r5D6YhzIy8dVIsrK0d2EG18CAwEAAQ==
-----END PUBLIC KEY-----
EOF

  rm_control_plane_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAuHBRRykLLq9rehMmLk4+
E9eTv4YC6FYA9GoeL+L5SqDhbyDZCmJVVGq01BR0cIxG6t+jRemu/ArJDyMCQDAW
sWWd05ZGAyldeoTfffsRcP4aHcnQNk0sEmeye1JzTn79fHACvUV/HY/76ZHjMiKn
X8Jz7mcq0rufM5WTBTyxRN7ZNwTtWPrms5pn+tXItUh+FYTxZhDNHGGuqhq4N4+F
GcEu9WnrQQ24bycshSk10BQ2bXv60Ie6n7HKC4CiwRWP2GTOrCEQE8zL9MRtlW2s
/eZU3fTA+/kEn0oQwDcdLh4ay7zSLzOMhZgrOnssd63BqsMIafqZCwAuwxncpKP5
65x6Cuw+XlImD9ZLcPR94tfME4KnK8luMen2JB/MFlzdy3WWHtOfnV8MhB9llLQp
/SrerYVNFUGs/6C6WdTES90M9F0eIvh0FKAxKLRV2nx0q1q4hsSaOZ/ERVAnuuuK
h8a2YZ5ac3EzSmsoICouoSxKvSD/js6/coqrnNQ5nkHeSkB7LcUK1Xm8rZZ7HZ3V
aPOT6f6jMVXRXTRP2vsIr22WVDsnKkRGEtHvqvP8QfHHUPXC60TOo/KcrbXmGXhM
5gJJFXI+fNmpgBV8GpF47BKRoQVIHkigpI2ZDd9HbG8/09x97XzDc+Pq/gc25bBh
qb0TsmQGevCPRnWDo+NxxbECAwEAAQ==
-----END PUBLIC KEY-----
EOF

  token_service_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA1YRiF3KPO3rOmRIJYXyY
R0ZV1RUZjVGzUQATuqR6ezXHQDEONxzYxt+9WZOlEHjmDz3KQR33N7PiLclvlzSD
27guDhzzdB6+iRI0VC20HdEWgU2xgsdNP21suBjM4ct/eEXAaTgtBeU3umfizsU6
ni/MVpk0+P1zlkejyqdWJNc6A5vw3ZVsb13dtij+z5bcYwkCBHkjzSM8H5WtJNMZ
fWvzPbDV4OT529NTZTSZN9Kb1m9R09uMaGA70qBa76kgG1d3yGhnuwH2P9IJcc9G
IoZQ54xilAErsha+n4DNMjarBSbRA7htFfyLq8dtlU8ZXiyE/yLfEsD5o1jUtQQ1
9TYHEceVZBXXPtzWQbN3F5MktGowo31YONMajNfErgR1Kkss4p901nTenoOi+o8Y
O3fY+q/emvpkX9Yu8jx5pqYNfDV5RTyzkUaIo5aUMxLezAGRua+f3Lwj6+0IGXal
GpErV8xb3IbCcn2BNUOMF6JThzuv9GtXzNFtankTJ1g+csSE8n2fRzasffMxRVXa
YQOMff3Ko6nEGZfbUgDfgDjwWa47zxtsBUemDAvGJdkdd4K0XAKC7PVAqCXLPj1I
NTLLlzYKHN7ISaQ4xiTS2oFKBIdA2zLJKyQOPC5UHl316NUOroApRHlAgRyewnnO
ta5A3qYDOc5LtVKMl9csWK8CAwEAAQ==
-----END PUBLIC KEY-----
EOF

  iam_sync_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAtxzgYZck65o+SU3TmDz8
hj6FtbABfRnRm1Nc8PfDf+U8j+E8bOFBIbbgkseJlBqJDoK0M3wDeFLF6nISMJf4
0Qo96TuWqNxMFsjt2fFVlZgisVq+W2VDMMgvWnaWOJQvG/muIw4rxiBp/M+SQoMK
cNtsowcF17sAIUsmgxwFx4jefwoNH8K6LbC3YnbtAogT6MG7LJmmEO8YHuINPU+P
+vAzMOW1k0keTqAMcqz77qYjdSCD0PP5QA07ewNIxyJGvgg4wN+cIh/tD8TJd/ah
WrPtLOLl5866AJIzWKTyfU38jK/JD+lgL1qjKWHfLBEegHPxRSPkyeW0QxTWpxhW
1mnRKaEOoaZsI/8WY9JNoBiXH7rrBqqsSmKzRn4PklDNkw22E/jSZqb1Fs2uZYCV
VvTPeecv9XSIzMNZ6L6Iz8IEwm1s9+vogw4L+h3ggY/EtcV6QpdpQ01K0/Cp3lZJ
mJbrvhwN3bDyT6/XuBXiTlPIost9RPQ+veAvLOu0cGgHrMzyczjh5UvFnUbdv1QX
qu35aV9i7XluI/sjw3GC1iwMYrkaWcMTdh98bjC0CN4/kA2eROYK92RIIz5koR2y
gnsREWedl0r3DGklg1I2jR4StmDWrFIJltuif6iRMHI9qVe+/4jAuJIi7mstHQR9
ubZFuy2wY/97AfXk2aNC/eECAwEAAQ==
-----END PUBLIC KEY-----
EOF

  iam_common_sa_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA60qBaJXb4bbDNOzApz/4
tO61zhcDgjYSe5n2hQ25HDD0i6WyYsJu0DGx8C2iLsKWUAOyiFsLt3AasXOZXm3d
ihmgNehAttrIUAsDA+HgFXODzHGptVs17rtglflSFhbkPE+qK1CLC1vtTVgwCccg
R3ytJV3bHVN3+iQI57pUgHjdEg1HXf7KsI7jVaxgoC3IfPLbtiC7pNXcUSYrXQvM
3INounA4Kc5ke2HB9hQAWfeLEcsBXOlNrG2MANatVoFzwn6oUGKYPvTTJTGagtzV
p6g+pkKIqYkevfVIQr7Z2ppElcZXPlGVaRUlYL98AQ8DOtWLKXUB0o3mwXqtI2CN
ZXVzzawV5AFofWNJDIlBMEtMywSFUDaJ6tDV1G0Jx4VkPs0DEUW8sGNoFJEE5l2F
OeOWWSj1cVuSQKSpwG5l6LYyQa1LanyEkJHqhdue6kN1yN4tHXxR7P0kwYOXJjZB
R+vby72Pzy2YPof+b2W2sqHZDLMs3VcbWFSsJPwP9XJLemwwGVKYJHzMk98gngtF
1EJWyUsXE/EvDKiRpLTvaWmlcoBYQuXQAFsGu6+gf0Y4pclhqrtxIRjB4qNfENn7
bcZZgBDB2I0jLdAWbzKjlnkDSJSf0FsEOkPowlaYf0E6gyh49PlP746Yix2FepVg
0NC5Xr4NI543a1QQ8jIW9+cCAwEAAQ==
-----END PUBLIC KEY-----
EOF

}
