from os import path
from jinja2 import Environment, FileSystemLoader, StrictUndefined


def getenv(loader=None):
    """
    Create Jinja2 env object
    """
    env = Environment(
        autoescape=False,
        trim_blocks=False,
        undefined=StrictUndefined,
        keep_trailing_newline=True,
        loader=loader,
    )
    return env


def render_config(config_path, context=None):
    """
    Renders the actual template.
    """
    if context is None:
        context = {}
    # Evaluate source dir for templates
    loader = FileSystemLoader(path.dirname(config_path))
    # Init Environment
    env = getenv(loader)
    try:
        return env.get_template(path.basename(config_path)).render(context)
    except Exception as exc:
        raise RuntimeError("'{exc_type}' while rendering '{name}': {exc}".format(
            exc_type=exc.__class__.__name__,
            name=config_path,
            exc=exc,
        ))
