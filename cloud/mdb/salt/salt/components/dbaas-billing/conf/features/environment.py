import tempfile
import shutil

from behave import register_type

from parse import with_pattern


@with_pattern(r'[^"]+')
def parse_param(text):
    """
    Actually don't parse anything,
    just regexp definition
    """
    return text


register_type(Param=parse_param)


def before_scenario(context, scenario):
    context.root = tempfile.mkdtemp()


def after_scenario(context, scenario):
    shutil.rmtree(context.root)


def after_step(context, step):
    if step.status == 'failed':
        if context.config.userdata.getbool('debug'):
            try:
                import ipdb as pdb
            except ImportError:
                import pdb
            pdb.post_mortem(step.exc_traceback)
