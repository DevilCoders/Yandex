try:
    from .version_actual import __version__
except ImportError:
    __version__ = 'dev'
