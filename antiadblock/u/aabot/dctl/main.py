import yaml
import os
from jinja2 import Template

input_file = "spec-template.yml"
output_file = "spec.yml"
context_file = "config.yml"

def main():
    with open(context_file) as yml_file:
        context = yaml.safe_load(yml_file)

    template = Template(open(input_file).read())
    rendering = template.render(context)

    with open(output_file, "w") as file:
        file.write(rendering)

    cmd = "ya tool dctl put stage " + output_file
    os.system(cmd)
