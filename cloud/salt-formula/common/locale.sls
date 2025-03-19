utf8_locale_is_present:
  locale.present:
    - name: en_US.UTF-8

utf8_locale_is_default:
  locale.system:
    - name: en_US.UTF-8
    - require:
      - locale: utf8_locale_is_present
