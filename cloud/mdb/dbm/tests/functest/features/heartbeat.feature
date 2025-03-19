Feature: Heartbeat works correctly

    Scenario: Valid heartbeat request works
        Given a deployed DBM
          And empty DB
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """
         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: vla
                switch: vla1-s1
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks: []
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            fqdn: vla1-0001.tst.yandex.net
            project: mdb
            geo: vla
            switch: vla1-s1
            allow_new_hosts: true
            generation: 2
            total_cores: 56
            total_memory: 269157273600
            total_io: 1468006400
            total_net: 1310720000
            total_ssd: 7542075424768
            total_sata: 0
            total_raw_disks: 0
            total_raw_disks_space: 0
            """
         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: vla
                cpu_cores: 32
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks: []
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            fqdn: vla1-0001.tst.yandex.net
            total_cores: 32
            """

    Scenario: Invalid heartbeat request doesn't work
        Given a deployed DBM
          And empty DB
         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                heartbeat: 1
            """
         Then we get response with code 422
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """

    Scenario: Valid heartbeat request with raw disks works
        Given a deployed DBM
          And empty DB
         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: vla
                switch: vla1-s1
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks:
                    - id: 11111111-1111-1111-1111-111111111111
                      max_space_limit: 10995116277760
                      has_data: false
                    - id: 22222222-2222-2222-2222-222222222222
                      max_space_limit: 10995116277760
                      has_data: true
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            fqdn: vla1-0001.tst.yandex.net
            project: mdb
            geo: vla
            switch: vla1-s1
            allow_new_hosts: true
            generation: 2
            total_cores: 56
            total_memory: 269157273600
            total_io: 1468006400
            total_net: 1310720000
            total_ssd: 7542075424768
            total_sata: 0
            total_raw_disks: 2
            free_raw_disks_space: 10995116277760
            total_raw_disks_space: 21990232555520
            """

         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: vla
                switch: vla1-s1
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks:
                    - id: 11111111-1111-1111-1111-111111111111
                      max_space_limit: 10995116277760
                      has_data: false
                    - id: 22222222-2222-2222-2222-222222222222
                      max_space_limit: 10995116277760
                      has_data: false
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            fqdn: vla1-0001.tst.yandex.net
            project: mdb
            geo: vla
            switch: vla1-s1
            allow_new_hosts: true
            generation: 2
            total_cores: 56
            total_memory: 269157273600
            total_io: 1468006400
            total_net: 1310720000
            total_ssd: 7542075424768
            total_sata: 0
            total_raw_disks: 2
            free_raw_disks_space: 21990232555520
            total_raw_disks_space: 21990232555520
            """

         When we issue "POST /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: vla
                switch: vla1-s1
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks:
                    - id: 11111111-1111-1111-1111-111111111111
                      max_space_limit: 10995116277760
                      has_data: false
            """
         Then we get response with code 200
          And body is:
            """
            {}
            """
         When we issue "GET /api/v2/dom0/vla1-0001.tst.yandex.net" with:
            """
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
            fqdn: vla1-0001.tst.yandex.net
            project: mdb
            geo: vla
            switch: vla1-s1
            allow_new_hosts: true
            generation: 2
            total_cores: 56
            total_memory: 269157273600
            total_io: 1468006400
            total_net: 1310720000
            total_ssd: 7542075424768
            total_sata: 0
            total_raw_disks: 1
            free_raw_disks_space: 10995116277760
            total_raw_disks_space: 10995116277760
            """
