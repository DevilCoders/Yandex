{% set secretsgram_secrets_dir = "/etc/ssl/yc" %}
{% set secrets_group = "yc-secrets" %}

{{ secrets_group }}:
  group.present:
    - members:
      - www-data

ca-certificates:
  yc_pkg.installed

secretsgram_secrets_dir:
  file.directory:
    - name: {{ secretsgram_secrets_dir }}
    - makedirs: True
    - mode: 750
    - user: root
    - group: {{ secrets_group }}
    - require:
      - yc_pkg: ca-certificates
      - group: {{ secrets_group }}

secretsgram_secrets_keys_dir:
  file.directory:
    - name: {{ secretsgram_secrets_dir }}/keys
    - makedirs: True
    - mode: 750
    - user: root
    - group: {{ secrets_group }}
    - recurse:
      - user
    - require:
      - group: {{ secrets_group }}
      - file: secretsgram_secrets_dir

# Set root password, see CLOUD-18509 for details #}
{% set root_password = "$6$QFWFSR9L$kN0GodOpJb645igCmnuSthgkYgxKTYZymNcRY.xjoAasmmPmSOcNvGizJNyh9m2kuNEQwfw2e4nTyHEDwtLvW0" %}
{% if grains["virtual"] != "physical" %}
{% set root_password = "$6$r/7bHNhS$OTRI1OCRY4Z2Ic6KeTyhGMd75m/FBZT4PxhgurLxbVQ4DUS3iv9t6ErgEmpApk5tJIimgHQx64fsLEbzfQlOx0" %}
{% endif %}
root-password:
  user.present:
    - name: root
    - password: {{ root_password }}
