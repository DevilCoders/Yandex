/home/teamcity/.dupload.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/home/teamcity/dupload.conf
    - user: teamcity
    - makedirs: True
    - mode: 0640
