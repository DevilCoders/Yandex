import random

from cloud.ai.nirvana.nv_launcher_agent.lib.resources.adjectives import ADJECTIVES
from cloud.ai.nirvana.nv_launcher_agent.lib.resources.animals import ANIMALS
from cloud.ai.nirvana.nv_launcher_agent.lib.resources.colors import COLORS


def generate_name(repeat_parts=False, separator='-',
                  lists=(ADJECTIVES, COLORS, ANIMALS)):
    name = []
    for l in lists:
        part = None
        while not part or (part in name and not repeat_parts):
            part = random.choice(l)
        name.append(part)
    return separator.join(name)


def generate(count=1, repeat_parts=False, unique_list=True, separator='-',
             lists=(ADJECTIVES, COLORS, ANIMALS)):
    names = []
    for i in range(count):
        name = None
        while not name or (name in names and not unique_list):
            name = generate_name(repeat_parts, separator, lists)
        names.append(name)
    return names
