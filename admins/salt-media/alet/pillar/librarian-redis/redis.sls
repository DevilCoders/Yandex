redis:
  server:
    enabled: True
    params:
      protected-mode: "no"
      tcp-backlog: 511
      maxmemory: 10g
      maxmemory-policy: noeviction
      "save 900": 1
      "save 300": 10
      "save 60": 10000
  sentinel:
    enabled: True
    params:
      protected-mode: "no"
