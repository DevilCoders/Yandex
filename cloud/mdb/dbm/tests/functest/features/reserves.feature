Feature: Resources reserves works correctly

    Scenario: Hypervisor selection is switched to busy mode on reserve fail

        Given a deployed DBM
          And empty DB
         When we issue heartbeat for vla1-0001.tst.yandex.net dom0
          And we issue heartbeat for vla2-0002.tst.yandex.net dom0

          And we launch container vla-123.db.yandex.net with 24 cores
          And we launch container vla-456.db.yandex.net with 16 cores
          And we launch container vla-789.db.yandex.net with 16 cores

         Then container info for vla-123.db.yandex.net contains:
            """
            fqdn: vla-123.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
         And container info for vla-456.db.yandex.net contains:
            """
            fqdn: vla-456.db.yandex.net
            dom0: vla2-0002.tst.yandex.net
            """
         And container info for vla-789.db.yandex.net contains:
            """
            fqdn: vla-789.db.yandex.net
            dom0: vla1-0001.tst.yandex.net
            """
