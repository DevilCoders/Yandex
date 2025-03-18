#!/skynet/python/bin/python
import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

# FIXME: copied from balancer config tag creation

import gencfg
import config
import gaux.aux_utils


def get_new_tag_name(repo):
    tag_prefix = "stable-{}-r".format(config.BRANCH_VERSION)
    last_release = gaux.aux_utils.get_last_release(repo, tag_prefix)
    tag_name = tag_prefix + str(last_release + 1)
    return tag_name


def create_new_tag(repo, tag_name):
    repo.create_tag(tag_name)
    print('Created: {}'.format(tag_name))


def delete_tag(repo, tag_name):
    repo.delete_tag(tag_name)
    print('Deleted: {}'.format(tag_name))


def show_help_message():
    print('Usage:')
    print('    ./utils/common/manipulate_gencfg_tags.py new_tag_name')
    print('    ./utils/common/manipulate_gencfg_tags.py create_tag <tag_name>')
    print('    ./utils/common/manipulate_gencfg_tags.py delete_tag <tag_name>')


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ('new_tag_name', 'create_tag', 'delete_tag'):
        show_help_message()
        return 0

    repo = gaux.aux_utils.get_main_repo(verbose=True)
    if sys.argv[1] == 'new_tag_name':
        print(get_new_tag_name(repo))
    elif sys.argv[1] == 'create_tag' and len(sys.argv) == 3:
        create_new_tag(repo, sys.argv[2])
    elif sys.argv[1] == 'delete_tag' and len(sys.argv) == 3:
        delete_tag(repo, sys.argv[2])
    else:
        print('Unknow format')
        show_help_message()
        return -1

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
