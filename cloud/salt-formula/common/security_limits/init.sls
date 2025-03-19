/etc/security/limits.d/core_dumps.conf:
  file.managed:
    - source: salt://{{ slspath }}/core_dumps.conf

core_security_limit:
  cmd.run:
    - name: prlimit --pid 1 --core=unlimited:unlimited
    - onlyif: 
      - prlimit --pid 1 -c |grep 0
