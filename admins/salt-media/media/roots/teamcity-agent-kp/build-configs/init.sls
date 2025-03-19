/home/teamcity/.dupload.conf:
  file.managed:
    - user: teamcity
    - source: salt://{{ slspath }}/common_dupload.conf
    - makedirs: True
    - mode: 0640

/home/teamcity/dupload_custom.conf:
  file.managed:
    - source: salt://{{ slspath }}/dupload.conf
    - makedirs: True
    - mode: 0644
