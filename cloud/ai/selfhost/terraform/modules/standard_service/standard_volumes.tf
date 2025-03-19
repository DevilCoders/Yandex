locals {
  standard_volumes = {
    all-configs = {
      hostPath = {
        path = "/etc"
        type = "DirectoryOrCreate"
      }
    }
    yandex-configs = {
      hostPath = {
        path = "/etc/yandex"
      }
    }
    var-log = {
      hostPath = {
        path = "/var/log"
      }
    }
    var-log-yc-ai = {
      hostPath = {
        path = "/var/log/yc/ai"
      }
    }
    var-lib-billing = {
      hostPath = {
        path = "/var/lib/billing"
      }
    }
    var-spool = {
      hostPath = {
        path = "/var/spool"
      }
    }
    var-log-fluent = {
      hostPath = {
        path = "/var/log/fluent"
      }
    }
    var-spool-push-client = {
      hostPath = {
        path = "/var/spool/push-client"
      }
    }
    push-client-config = {
      hostPath = {
        path = "/etc/yandex/statbox-push-client"
      }
    }
    solomon-agent-config = {
      hostPath = {
        path = "/etc/solomon-agent"
      }

    }
    logrotate-config = {
      hostPath = {
        path = "/etc/logrotate.d"
      }
    }
    data-sa-keys = {
      hostPath = {
        path = "/etc/yc/ai/keys/data-sa"
      }
    }
    root = {
      hostPath = {
        path = "/"
      }
    }
    var-run = {
      hostPath = {
        path = "/var/run"
      }
    }
    sys = {
      hostPath = {
        path = "/sys"
      }
    }
    var-lib-docker = {
      hostPath = {
        path = "/var/lib/docker"
      }
    }
    dev-disk = {
      hostPath = {
        path = "/dev/disk"
      }
    }
    run-prometheus = {
      hostPath = {
        path = "/run/prometheus"
      }
    }
    etc-secrets = {
      hostPath = {
        path = "/etc/secrets"
      }
    }
    unified-agent-data = {
      hostPath = {
        path = "/var/lib/unified-agent-data"
        type = "DirectoryOrCreate"
      }
    }
  }
}