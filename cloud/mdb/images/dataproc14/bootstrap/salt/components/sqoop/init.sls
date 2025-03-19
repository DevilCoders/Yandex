include:
    - .packages
{% if salt['ydputils.is_masternode']() %}
/root/.java.policy:
    file.managed:
        - source: salt://{{ slspath }}/conf/.java.policy
        - user: root
        - group: root
        - mode: '0644'
        - makedirs: true
{% endif %}
