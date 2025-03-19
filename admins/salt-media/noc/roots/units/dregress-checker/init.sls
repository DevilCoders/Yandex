/etc/dregress-checker/dregress-checker.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/dregress-checker/dregress-checker.yaml
    - template: jinja
    - makedirs: True
