{% if grains["yandex-environment"] != "production" %}
eatmydata:
  pkg.installed

/etc/ld.so.preload:
  file.managed:
{% if grains["oscodename"] == "xenial" %}
    - contents: |
        /usr/lib/x86_64-linux-gnu/libeatmydata.so
{% else %}
    - contents: |
        /usr/lib/libeatmydata/libeatmydata.so
{% endif %}

{% else %}
DO NOT USE IN PRODUCTION!!!:
  cmd.run:
    - name: echo "!!!ATTENTION!!! Do not use in production!!!"; exit 1
{% endif %}
