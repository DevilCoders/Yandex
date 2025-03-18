from nirvana_api.blocks import BaseBlock, fix_attr_name
from functools import wraps


def apply(block, args, kwargs, output_name, input_names):
    if len(args) > len(input_names):
        raise RuntimeError('{} args to block {} (expected at most {})'.format(
                           len(args), block.__name__, len(input_names)))
    for arg, name in zip(args, input_names):
        kwargs[fix_attr_name(name)] = arg
    return getattr(block(**kwargs).outputs, fix_attr_name(output_name))


def as_function(block, output_name, *input_names):
    @wraps(block)
    def fun(*args, **kwargs):
        return apply(block, args, kwargs, output_name, input_names)
    return fun


class FunctionalBlock(BaseBlock):
    class __metaclass__(type):
        @property
        def signature(self):
            return self.output_names[:1] + self.input_names + self.parameters

    @classmethod
    def f(block, *args, **kwargs):
        return apply(block, args, kwargs, block.signature[0], block.signature[1:])


__all__ = ['as_function', 'FunctionalBlock']
