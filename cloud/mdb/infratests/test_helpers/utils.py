import functools
import json
import yaml

from jinja2 import Template

from cloud.mdb.infratests.test_helpers.dataproc import get_default_dataproc_version, get_default_dataproc_version_prefix


def context_to_dict(context):
    """
    Convert behave context to dict representation.
    """
    result = {}
    for frame in context._stack:  # pylint: disable=protected-access
        for key, value in frame.items():
            if key not in result:
                result[key] = value

    return result


def get_step_data(context, format: str = None):
    """
    Return step data deserialized from YAML or JSON representation and processed by template engine.
    """
    if context.text:
        # make this function available in jinja templates with preset context argument
        context.get_default_dataproc_version = functools.partial(get_default_dataproc_version, context)
        context.get_default_dataproc_version_prefix = functools.partial(get_default_dataproc_version_prefix, context)
        rendered_text = render_template(context.text, context_to_dict(context))
        if format == 'text':
            return rendered_text
        elif context.text.lstrip().startswith('{'):
            return json.loads(rendered_text)
        else:
            return yaml.safe_load(rendered_text)
    return {}


def render_template(template, template_context):
    """
    Render jinja template
    """
    return Template(template).render(**template_context)


def render_text(context, template):
    """
    Return rendered template
    """
    return render_template(template, context_to_dict(context))
