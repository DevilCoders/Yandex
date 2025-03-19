"""
DBaaS Internal API Config
"""
from collections import OrderedDict
{% for section, options in salt['pillar.get']('data:internal_api:config', {}) | dictsort %}
{%     if options is mapping %}
{{ section|upper }} = {
{%         for option, value in options | dictsort %}
    {{ option|python }}: {{ value|python }},
{%         endfor %}
}
{%     else %}
{{ section|upper }} = {{ options|python }}
{%     endif %}
{% endfor %}
