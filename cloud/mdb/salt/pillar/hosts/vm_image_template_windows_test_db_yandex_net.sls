mine_functions:
    grains.item:
        - id
        - role
        - dbaas
        - ya
        - virtual

yandex:
    environment: dev

data:
    windows:
        users:
            Administrator:
                password: 'qweasd123!@#'
    solomon_cloud:
        sa_private_key: 'soprivatesuchakey'
    sqlserver:
        databases:
            test1: {}
            test2: {}
        users:
            sa:
              password: 'SuperDatabase789'
              sid: '0x01'
            user1:
              password: 'password1'
              sid: '0x48f320ea02d44151b73c59b97e16f095'
              dbs:
                test1:
                  roles: [ db_owner ]
            user2:
              password: 'password2'
              sid: '0x867a16264bee40d4818a865db46a21f1'
              dbs:
                test2: 
                  roles: [ db_datareader ]
        repl_cert:
          cert.key: |
            -----BEGIN PRIVATE KEY-----
            MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC4ivfrPM5UNOL0
            0P34hEeFQBQoxnRV/BmaeyzWCEQ+9OK6RtPawV7gQn5r6/ciOOiSX+D9S07T/BBY
            tbVS4TFmlMhEsZyhc0rdjzukIWxXPplQoGI/LAYtFq8FEPqfKfyYTNP9LK9KdAfY
            FhZZiSD0je9Vyaf9fQrLCJixdTR4d0bLcI0UnKY1jF4YBGhBhk7epigj6QKa3b7k
            Gt2L4fkgolZD2bXWMaYWkPivipQg6PrHtUrIRlBs0xb9IC8OWMUwUnNWrMnsJ+Iu
            GTNJ8iTjS3IAG+i7RSdDe2iAChy+Cr95NbPHZtOms5fS2T9thkhRsuxvFwRqR1Yh
            NIZly6tjAgMBAAECggEBAKTAxJdKIPaChMCGOMb+6Y8n5xeEHWaKfB0zrJKkrLD8
            FdKx2v+otVhHZPBaqLbv0YqkeIwKCKUumzVnfCszCtSHLchOPQTSllr5PgjJIh09
            sMiPd0boudbVMom2X9lrNayOoo+brh8tObeL+IYU68wJT0vqjcS8Nr+OvCtp9N86
            MQ2h9hWrLCBWcGLmJNsMUB7iDK21cc5txCu99/TYS07S4bquUCSDoSLGX53E7i/d
            xCo3lj/nI6S0BnK0xHzRzYv7Kzrb64bmyGffQOwoK2t2lPPatbaWPheJUrgOy+A0
            gFnjFFkoWCn1cQ9ssThMicD0aXyLFkDXtUBYDLYkAekCgYEA4whBhqgX5m63VUBZ
            ajlUPRIMtKR4/J8CXw7/L0xP8gMmodeY6J7HJbKjYma3zFTNX4kMg3s7nf7Q5BPP
            97Ot2whDHDZ9D40X9PUp/f9y7pD4j7QaCThwxQLOk7A2zomD+7jIyLuugwKQ1XFd
            CMhtmfuNPl2/wiVWrL+D21R4AX0CgYEA0BbYdqimAwP50LjfIE/5I/lHu9vxEI0L
            DOdC0wLelVq4bbARM9VXsFjne04enFHj31xpvpm7ZRqjCsgL2G+ntvPB+CPg7Oqr
            xKRh6hf6a7mR6OOC3/BEZ3Fc2jI+SMDg8qd5nm+aVDpHeSDmrVhUxt4AoswCnQUJ
            oJLeRLM59l8CgYEAoBgETPZf1Ciis0UrSFKg6me9+ew5PLrMK0gh/rJrdZdvOJUV
            yIOenyC3Qf55AWeTYxj7cAQIaEN1/j9SWbOkl13eJRjvW3X9PMldETV/UGd+P6ea
            7IGWU/bEwTRzkXU7UthdDd/7EXtPspO8LSNY1kqisSXewQMfebWbP6Dy9ukCgYB6
            a7ZVuMpSI9iortq2C9arD1DgeHjOH+xMXxG1So7gl0rBXUABmpXILcWhiUgA9zx0
            f83GUHMq/AzHnwE5CBdHTwjij1kYiMcdHQhtzEWLctUgihjI3nWf1dWmmMCD1NWC
            bHzkdQv6WX8LCAwuFzVo+dqt7A/tiLUtPrp8+GQ1SQKBgE6cQsgeJrINljPKiXPw
            aEH7WxrxZop1Tq1BrT4TPNi/awicVO/gvRcNFaE8Pg0qYf5WdxxSPTthHQSxf1bb
            MrG/+FWi2MK+4AFXkpz+Ek0LVw66yHMawVmYXwyS5pMSxOdtyVrcl6emEoXvzxD/
            QsPwT7rJZGfdBNYJoXls9kVz
            -----END PRIVATE KEY-----
          cert.crt: |
            -----BEGIN CERTIFICATE-----
            MIIDizCCAnOgAwIBAgIUMi6l94fI8n0uyeK9eCJY+7upLoMwDQYJKoZIhvcNAQEL
            BQAwVTELMAkGA1UEBhMCUlUxDzANBgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9z
            Y293MQ8wDQYDVQQKDAZZYW5kZXgxEzARBgNVBAMMCnJlcGwubG9jYWwwHhcNMjAw
            OTEyMjIyNTM1WhcNMjQwNjA4MjIyNTM1WjBVMQswCQYDVQQGEwJSVTEPMA0GA1UE
            CAwGTW9zY293MQ8wDQYDVQQHDAZNb3Njb3cxDzANBgNVBAoMBllhbmRleDETMBEG
            A1UEAwwKcmVwbC5sb2NhbDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
            ALiK9+s8zlQ04vTQ/fiER4VAFCjGdFX8GZp7LNYIRD704rpG09rBXuBCfmvr9yI4
            6JJf4P1LTtP8EFi1tVLhMWaUyESxnKFzSt2PO6QhbFc+mVCgYj8sBi0WrwUQ+p8p
            /JhM0/0sr0p0B9gWFlmJIPSN71XJp/19CssImLF1NHh3RstwjRScpjWMXhgEaEGG
            Tt6mKCPpAprdvuQa3Yvh+SCiVkPZtdYxphaQ+K+KlCDo+se1SshGUGzTFv0gLw5Y
            xTBSc1asyewn4i4ZM0nyJONLcgAb6LtFJ0N7aIAKHL4Kv3k1s8dm06azl9LZP22G
            SFGy7G8XBGpHViE0hmXLq2MCAwEAAaNTMFEwHQYDVR0OBBYEFLbrjH1ed5pNpVb3
            iqy5pkT1KjOhMB8GA1UdIwQYMBaAFLbrjH1ed5pNpVb3iqy5pkT1KjOhMA8GA1Ud
            EwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAJfBD5qhPamKtJJfKOyj7ZaE
            Xmqcucj7BrXIhdKKCLLD0AIma5y5ytcHU9YGs7Y6YLLYtjjncYywYqSEj9rhcelb
            V+PVZduZ/PuPg89Si2IS0bzkX6vgDFGHcrt6CzPOJsMqPGPVs5dLq1hybd/q1GSI
            Kn9jFXKGszLrJCQS1cJZuWUK3fBF3G4ZGOiIJotm2k/VGORGFhvEXiahp3S8O7QS
            H+LmXgFvEqcS3bl0av/SNthAIurqVlEorM9Fp015Q2BeywpVIvrSupgSLonO5Nmf
            0Jfd7/b474MyhQyYsdYelSH7ahiwivwowHAvco7sh13sJu7gm1IO8GvcCeBJf7k=
            -----END CERTIFICATE-----

    runlist:
        - components.sqlserver_cluster

cert.key: |
  -----BEGIN PRIVATE KEY-----
  MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC3/dA/T/VZC8ag
  JxeNIeUEukS75mREIWdnHfefZeyd5XX9BqbqhrgbLDhXpNq4vjJwnEQMui+kD7Lv
  TMBw4qnrjAU670gcmcLtq5QPq/xGifz1hBZQ0xlwlmZsaL4HpGzZVLzxddo3bgEv
  ACns724TiU10fo4HBr50FITq+GdLDlIwE4YWWUQezadECdfaZtnGkZ5IuHyYZpu0
  3J5n+IEbHh/kFCOC0wHkpiSFthXDcyrqJrfW904FPbUBFyNCmknXKZJV/GBe6VUU
  TLCz//ewSCZ9x/jq79MAUpwxZX40pw2YBabbdruub0UAaMya8OeSy/7WooKCEo0k
  ToPW/KdHAgMBAAECggEAAjk3AwzcUi8qP1U5m6MFOYFmwz6Nh6U/sqdSDsMUkPPs
  8RnbeooWP33RUH3VhtYFlgbNa7n0SW9HIk0zJioXE5mlloi9vlq9WFHygB379n5E
  aFMoUeG7NPkcU8MQSNdb2WSExAZAlNrneMHvv8VE90d5gCbnYH5cXtrAoHZQCUAl
  yR6iVBdzvb5qEOlZBFJEB7gsMQzN6QiB15fE6qLVRyrtwYh9jJtxgQOtTpY/CZeF
  m67Xu0e6AZ4aYT3HtZkXaAxlresTVDgqi9irQ7EMrPmHw2KdYDpFMWEuJP8Nvhne
  hjoxcsX6haBh7ECs7tfy/pV5oDJmSujv0HO5V/doQQKBgQDuyG1LcbLRL0joll82
  51jsA59alnSXliDXp5JtsrAkDlNNjl45u41z7fL0RcaogfE7rP5aaQX2k2ZOsGxW
  CbyD0Gqs7DIc2m1OHl2RkGUZ3BXXVFJV4lIcjyIAZ29xmWLhlD7mBFmNfmkSafwp
  cxmKOdfJjRn6hjsS8XzJpPjTwwKBgQDFQgTKkflAH/P/ivJo0GCQB2fzI7iYIdB5
  B5jk8NadDLm3CLOfugWEw+TcxUBbNQs+XVXOhWP/F00Om5r6rPiZssTNZF/XI62O
  ttWM6+hS1MbB781PXhBS//Adq9YiNosUtff9S6mw5q5skGmrEZOJx/5726BKyvWg
  iZhCDZH6LQKBgGF5XexbSJTOCaQwpkOiYxFNVULEgmnS9isyk7rGI9NEKctSH7LP
  linJ69C9nx/wk4o5z0cW8kE64Jukt0TUCqmAbduTO829eLtX9/hRCoRo7F2PZWD7
  12sjHs0tbDfiVpQhUR/SjMAO51OGAlLlUnqzrBvxxrYXD4xnGfqypoTDAoGAWbfd
  VgVuyKu/3srj1qhwQxIbRYtJumgm2eWKtRaVhnjzPzzF9jA1sl0nCwLsco99va/W
  B0jEYpHGio5bvwhNShr1SDYPIVso5XBjUOU7lfERNcRMLX0rD5U5MUWm74i/WXwq
  fxqa48sIKnjElvo3kMNWDB+omFwXvBzZE7H4Y0ECgYA8Q8NSw1W02WL/tRmQIhAQ
  92vKkRE/bWI+b/IUCqNlEuN1fYfKidb4EZDbDWlipo5hvQ0doMmEqcDu3Q9xdMUq
  +YEdBH643wcR7pIMtYqfC5LwrAbuWg8sZkTgcOVne9AepuBICmDV+wabABEGYhl2
  PnMqQdA1c8UdoK499169+Q==
  -----END PRIVATE KEY-----

cert.crt: |
  -----BEGIN CERTIFICATE-----
  MIID5jCCAs6gAwIBAgIUB0hFaChEwO3Shg5j2bjI1nF+Yr4wDQYJKoZIhvcNAQEL
  BQAwJTEjMCEGA1UEAwwadm0taW1nLXdpbi10LmRiLnlhbmRleC5uZXQwHhcNMjAw
  OTExMTYyMDU5WhcNMjQwNjA3MTYyMDU5WjAlMSMwIQYDVQQDDBp2bS1pbWctd2lu
  LXQuZGIueWFuZGV4Lm5ldDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
  ALf90D9P9VkLxqAnF40h5QS6RLvmZEQhZ2cd959l7J3ldf0GpuqGuBssOFek2ri+
  MnCcRAy6L6QPsu9MwHDiqeuMBTrvSByZwu2rlA+r/EaJ/PWEFlDTGXCWZmxovgek
  bNlUvPF12jduAS8AKezvbhOJTXR+jgcGvnQUhOr4Z0sOUjAThhZZRB7Np0QJ19pm
  2caRnki4fJhmm7Tcnmf4gRseH+QUI4LTAeSmJIW2FcNzKuomt9b3TgU9tQEXI0Ka
  SdcpklX8YF7pVRRMsLP/97BIJn3H+Orv0wBSnDFlfjSnDZgFptt2u65vRQBozJrw
  55LL/taigoISjSROg9b8p0cCAwEAAaOCAQwwggEIMB0GA1UdDgQWBBSo4RAtvL47
  uf9Exf0fLn5eV7CyHTAfBgNVHSMEGDAWgBSo4RAtvL47uf9Exf0fLn5eV7CyHTAJ
  BgNVHRMEAjAAMAsGA1UdDwQEAwIFoDBhBgNVHREEWjBYgix2bS1pbWFnZS10ZW1w
  bGF0ZS13aW5kb3dzLXRlc3QuZGIueWFuZGV4Lm5ldIIadm0taW1nLXdpbi10LmRi
  LnlhbmRleC5uZXSCDHZtLWltZy13aW4tdDAsBglghkgBhvhCAQ0EHxYdT3BlblNT
  TCBHZW5lcmF0ZWQgQ2VydGlmaWNhdGUwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsG
  AQUFBwMCMA0GCSqGSIb3DQEBCwUAA4IBAQAmyOQMjjeh4+pBNKlKMArZIOozKFS/
  DGbqQ2aS+wMGhID0G+EOej4EkPaOCr0guIk+SJ6q7NhwIlDJejtl3pEB0HAQbltt
  qKgY1CKRFw6SZSfJziZfQ4CCAyBs2XTJH3mJBmUrp5Q2BFKlTeNlrQUSOWgAVDOJ
  cyTHkiFTDcEvXQluTUKeD6pt3KmjEseFqN2+sCpPdF4kCKWKLAFbgd/WHaG9sF97
  SnosELRVb6QaIpAB3Rh35FrH9E3h0jbS6pmC+vB5UOFGfTkyFxCl8wxT2kLhkcPa
  DRZr2nM4J9UL/lzokwWwUKp9d3Ucm3lCLy2ELGbdFxDbrg/73J7n9Hyo
  -----END CERTIFICATE-----
