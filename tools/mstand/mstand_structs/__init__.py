from .squeeze_versions import SqueezeVersions
from .lamp_key import LampKey

# For not failing flakes "imported but unused" test
__all__ = [
    'SqueezeVersions',
    'LampKey',
]
