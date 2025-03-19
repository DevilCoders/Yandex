hw_watcher:
  bot:
    token: {{ salt.yav.get('sec-01epynddbcmerbgpr0rmgm2y5n[robot-nocdev-hw_oauth-token]') | json }}
  mail: nocdev-root@yandex-team.ru
  initiator: nocdev-root
  enable_module:
    - disk
    - link
    - mem
    - ecc
    - bmc
  reaction:
    default: [ mail, bot-needcall ]
    ecc:  [ mail, bot-needcall ]
    mem: [ mail, bot-needcall ] 
    disk:
      default: [ mail, bot ]
      failsafe: [ mail, bot ]
      system: [ mail, bot ]
      
