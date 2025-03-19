{% if "p2p_monitoring" in pillar %}

{%- from slspath + "/map.jinja" import p2pm with context %}

{% set script = "/usr/local/bin/monrun_p2p_monitoring.sh" %}
{{ script }}:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/monrun_p2p_monitoring.sh
    - mode: 0755

{%- if p2pm %}
  {%- for name in p2pm %}
    {%- if name.startswith("age") %}
      {%- if "arguments" not in p2pm[name] %}
        {%- set args = [ "{0}".format(p2pm[name].get("utime", "")) ] %}
        {%- for dir,fls in p2pm[name].files.iteritems() %}
          {%- if "utime" in fls %}
            {%- set p = "utime={0}^dir={1}".format(fls["utime"], dir) %}
            {%- do args.append("{0}^files={1}".format(p,",".join(fls["names"])))%}
          {%- else %}
            {%- do args.append("dir={0}^files={1}".format(dir,",".join(fls)))%}
          {%- endif %}
        {%- endfor %}
        {%- do p2pm[name].setdefault("arguments", " ".join(args)) %}
      {%- endif %}
    {%- endif %}
  {%- endfor %}

/etc/monrun/conf.d/p2p_monitoring.conf:
  file.managed:
    - makedirs: True
      contents: |
        {%- for name, data in p2pm.iteritems() %}
        [p2p_{{ name }}]
        {%- set check = "client" if name=="client" else name %}
        command = {{ script }} {{ check }} {{ data.get("arguments", "") }}
        execution_interval = {{ data.get("execution_interval", 120) }}
        {%- endfor %}

regenerate-monrun-tasks:
  cmd.wait:
    - name: /usr/sbin/regenerate-monrun-tasks
    - require:
      - file: /etc/monrun/conf.d/p2p_monitoring.conf
{% endif %}

{% else %}
p2p_monitoring pillar NOT DEFINED:
  cmd.run:
    - name: echo "p2p_monitoring pillar NOT DEFINED";exit 1
{% endif %}
