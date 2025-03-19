/usr/local/bin/agent.py:
  file.managed:
    - mode: 755
    - user: root
    - source: salt://units/walle_agent/files/agent.py
 
/etc/cron.d/walle_cms_agent:
  file.managed:
    - user: root
    - mode: 644
    - source: salt://units/walle_agent/files/walle_cms_agent.cron
