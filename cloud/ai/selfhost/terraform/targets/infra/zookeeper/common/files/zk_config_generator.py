import json
import sys


def main():
    input = json.load(sys.stdin)
    instance_number = input['instance_number']
    template_path = input['template_path']
    env = input['environment']

    output = {}
    with open(template_path, 'r') as content_file:
        content = content_file.read()

    if env == 'preprod':
        host_str_template = "\nserver.{0}=zookeeper-{1}-{0}.datasphere.cloud-preprod.yandex.net:2888:3888"
    else:
        host_str_template = "\nserver.{0}=zookeeper-{1}-{0}.datasphere.cloud.yandex.net:2888:3888"

    for instance_idx in range(int(instance_number)):
        content += host_str_template.format(instance_idx + 1, env)

    output['rendered'] = content

    print(json.dumps(output))


if __name__ == '__main__':
    main()
