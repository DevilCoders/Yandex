/etc/monrun/salt_postfix-queue/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True

fix_perms:
  file.directory:
    - name: /var/lib/postfix
    - user: postfix
    - group: postfix
    - recurse:
      - user
      - group

/etc/monrun/salt_postfix-queue/postfix-queue.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/postfix-queue.sh
    - makedirs: True
    - mode: 755
