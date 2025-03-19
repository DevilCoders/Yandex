/etc/yandex/clickphite:
  file.directory:
  - dir_mode: 755

# HINT: from storage-secure
/etc/yandex/clickphite/clickphite-production.properties:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/clickphite/clickphite-production.properties
    - template: jinja

/etc/yandex/clickphite/dictionaries.xml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/clickphite/dictionaries.xml

/etc/yandex/clickphite/conf.d/:
  file.recurse:
    - dir_mode: 755
    - file_mode: '0644'
    - source: salt://{{ slspath }}/files/etc/yandex/clickphite/conf.d/
