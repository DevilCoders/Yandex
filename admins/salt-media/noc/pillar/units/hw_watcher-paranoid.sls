hw_watcher:
  bot:
    token: {{ salt.yav.get('sec-01epynddbcmerbgpr0rmgm2y5n[robot-nocdev-hw_oauth-token]') | json }}
  mail: nocdev-root@yandex-team.ru
  enable_module:
    # - link - выключаем по задаче NOCDEV-6172
    - disk
    - mem
    - ecc
    - bmc
  reaction:
    default: [ mail, bot-needcall ]
    ecc:  [ mail, bot-needcall ]
    mem: [ mail, bot-needcall ] 
    disk:
      default: [ mail, bot-needcall ]
      failsafe: [ mail, bot-needcall ]
      system: [ mail, bot-needcall ]

