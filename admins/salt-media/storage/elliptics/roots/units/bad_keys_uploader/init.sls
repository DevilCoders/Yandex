{%- from slspath + "/map.jinja" import bad_keys_uploader_vars with context -%}

/etc/bad_keys_uploader/bad_keys_uploader.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/bad_keys_uploader/bad_keys_uploader.yaml
    - template: jinja
    - makedirs: true
    - context:
      vars: {{ bad_keys_uploader_vars }}
