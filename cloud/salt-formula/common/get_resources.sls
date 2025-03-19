include:
  - cli

{% macro render_resources(resources_set, all_resources) %}
  {%- for resource_type in resources_set %}
    {%- if resource_type in all_resources %}
        # resources_set[resource_type] if it is generated using jinja for on empty list
        {%- if resources_set[resource_type] != None %}
            {%- for item in resources_set[resource_type] %}
                {%- if item in all_resources[resource_type] %}
{{ item }}:
  {{ all_resources[resource_type][item]|yaml(False)|indent() }}
                {%- endif %}
            {%- endfor %}
        {%- endif %}
    {%- endif %}
  {%- endfor %}
{% endmacro %}

{%- set all_resources = pillar['list_resources'] -%}
{%- set resources_set = pillar['resources_sets'][resources_set_name] -%}

{{ render_resources(resources_set, all_resources) }}

{%- if additional_resources_set is defined -%}
{{ render_resources(additional_resources_set, all_resources) }}
{%- endif -%}

