'''
This method provides possibility multimethods:  function that has multiple versions, distinguished by the type of the
arguments. So it is not needed to have a lot of if statements inside one big function. Just define function with
@multimethod decorator to define it behaviour on argument of different types.

Example:

    @multimethod(Rectangle, Ellipse)
    def intersect(r, e):
        print('Rectangle x Ellipse [names r=%s, e=%s]' % (r.name, e.name))

    @multimethod(Shape, Shape)
    def intersect(s1, s2):
        print('Shape x Shape [names s1=%s, s2=%s]' % (s1.name, s2.name))


'''
import typing as tp


class _MultiMethod:
    """Maps a tuple of types to function to call for these types."""

    def __init__(self, name: str) -> None:
        self.name = name
        self.typemap: tp.Dict[tp.Tuple[tp.Any, ...], tp.Callable[..., tp.Any]] = {}

    def __call__(self, *args: tp.Tuple[tp.Any]) -> tp.Any:
        types = tuple(arg.__class__ for arg in args)
        try:
            return self.typemap[types](*args)
        except KeyError:
            raise TypeError('No match %s for types %s' % (self.name, types))

    def register_function_for_types(self, types: tp.Tuple[tp.Any, ...], function: tp.Callable[..., tp.Any]) -> None:
        if types in self.typemap:
            raise TypeError("Duplicate registration")
        self.typemap[types] = function


# Maps function.__name__ -> _MultiMethod object.
_multi_registry: tp.Dict[str, _MultiMethod] = {}


def multimethod(*types: tp.Tuple[tp.Any, ...]) -> tp.Callable[[tp.Callable[..., tp.Any]], _MultiMethod]:

    def register(function: tp.Callable[..., tp.Any]) -> _MultiMethod:
        name = function.__name__
        multi_method_obj = _multi_registry.get(name)
        if multi_method_obj is None:
            multi_method_obj = _multi_registry[name] = _MultiMethod(name)
        multi_method_obj.register_function_for_types(types, function)
        return multi_method_obj

    return register

__all__ = ['multimethod']
