{% if salt['pillar.get']("icecream:agent:enabled") %}
include:
  - .service
  - .configs
  {% if salt['pillar.get']("icecream:agent:monitoring") %}
  - .monitoring
  {% endif %}
{% else %}
icecream_agent_not_enabled:
  cmd.run:
      - name: echo "icecream:agent:enabled set to False, or memory leaks on salt master!";exit 1
{% endif %}
