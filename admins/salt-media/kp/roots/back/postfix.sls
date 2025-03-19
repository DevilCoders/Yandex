/etc/postfix/canonical_map:
  yafile.managed:
    - source:
      - salt://{{ slspath }}/files/canonical_map

/etc/postfix/main.cf:
  file.append:
    - text:
      - "canonical_maps = regexp:/etc/postfix/canonical_map"

'postmap /etc/postfix/canonical_map':
  cmd.run

postfix_reload:
  service.running:
    - name: postfix
    - enable: True
    - reload: True
