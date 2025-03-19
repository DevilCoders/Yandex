import re
import random
from typing import Dict, List


var_desc_regex = re.compile(r'\{.+?=.+?\}')


def generate_text(template: str, mapping: Dict[str, List[str]]):
    text = template
    for var in var_desc_regex.findall(template):
        var_name, source = var[1:-1].split('=')
        value = random.choice(mapping[source])
        text = text.replace(var, value, 1)
    return text
