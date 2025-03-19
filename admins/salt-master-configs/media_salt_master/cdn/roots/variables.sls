{% load_yaml as m %}
robot: "robot-media-salt"

{% if "test" in  grains["yandex-environment"] %}
masters_group: "strm-test-salt"
tmpfs: "1G"
{% else %} # production
masters_group: "strm-salt"
tmpfs: "4G"
{% endif %}

{% endload %}

