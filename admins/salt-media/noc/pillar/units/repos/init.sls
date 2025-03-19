{% from slspath + "/repo_project.jinja" import project_sources with context %}
{% from slspath + "/repo_custom.jinja" import custom_sources with context %}

repos:
  sources: {{ custom_sources }}

repos_project:
  sources: {{ project_sources }}
