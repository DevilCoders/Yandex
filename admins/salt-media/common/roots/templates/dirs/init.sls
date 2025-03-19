{% for dirs, perms in pillar.get('dirs', {}).iteritems() %}
{{ dirs }}:
  file.directory:
    - makedirs: True
    - user: {{ perms['user'] | default('root') }}
    - group: {{ perms['group'] | default('root') }}
    - mode: {{ perms['mode'] | default('755') }}
{% endfor %}
