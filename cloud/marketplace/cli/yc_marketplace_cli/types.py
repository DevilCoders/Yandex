import argparse
import re
from functools import partial
from typing import Callable


class ArgumentType:
    @staticmethod
    def regex(pattern: str) -> Callable:
        def check(value, pat):
            if not re.match(pat, value):
                raise argparse.ArgumentTypeError("Value `{}` doesn't match pattern `{}`.".format(value, pattern))

            return value

        return partial(check, pat=pattern)
