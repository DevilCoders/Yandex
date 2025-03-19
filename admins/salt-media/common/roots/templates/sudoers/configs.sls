{% if "sudoers" in pillar %}

{% set sudoers = salt.pillar.get("sudoers") %}

{% for _local_name_ in sudoers %}

{% set _curr_project_ = sudoers.get(_local_name_) %}
{% set _result_configs_ = {} %}

{% if "group" in _curr_project_ %}
  {% do _result_configs_.update({"group": _curr_project_.group}) %}
{% endif %}

{% if "mail_account" in _curr_project_ %}
  {% do _result_configs_.update({"mail_account": _curr_project_.mail_account}) %}
{% endif %}

/etc/sudoers.d/{{ _local_name_  }}:
  file.managed:
    - source: salt://templates/sudoers/files/etc/sudoers.d/sudo.tmpl
    - template: jinja
    - context:
        mail_account: {{ _result_configs_.mail_account  }}
        group: {{ _result_configs_.group  }}

{% endfor %}



{% else %}

sudoers pillar NOT DEFINED:
  cmd.run:
    - name: echo "Should defile sudoers pillar!"; exit 1

{% endif %}

