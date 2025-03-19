#!/usr/bin/python2.7
"""
mysql-replace-configs-4 (4 Series). Replaces default mysql configs
"""
import argparse
from mysql_configurator.grants import ConfigWorker
from mysql_configurator import load_config


def main():
    """main"""
    argsparser = argparse.ArgumentParser(
        description='mysql-replace-configs (4 series).'
        'Without args use old way through secdist for users config.',
        epilog='*: default repository path is'
        'git@github.yandex-team.ru:salt-media/<project_name>-secure. '
        'Repository path can be overrided in /etc/mysql-configurator/grants.conf')
    argsparser.add_argument('-v', '--verbose', help='use verbose output',
                            default=False, action='store_true')
    argsparser.add_argument('-c', '--cached', help='use cached configuration',
                            default=False, action='store_true')
    argsparser.add_argument('-n', '--dryrun', help='dryrun(do not do anything, just show changes)',
                            default=False, action='store_true')
    argsparser.add_argument('-i', '--init',
                            help='initial setup. ACHTUNG! Be careful with this.'
                            'It will setup new datadir if it not existst,'
                            'restarts mysql and updates grants.'
                            'Use it only if you want setup a new mysql server.',
                            default=False, action='store_true')
    argsparser.add_argument('-a', '--apihost',
                            help='define host to get grants configs path(def: c.yandex-team.ru)',
                            default='c.yandex-team.ru', action='store')

    arguments = argsparser.parse_args()
    if arguments.verbose:
        print("Arguments %s", arguments)

    if not arguments.verbose:
        config = load_config(force_lvl='INFO')
    else:
        config = load_config()

    if arguments.cached:
        config.grants.cached = True

    # overrides from arguments
    if arguments.dryrun:
        config.grants.dryrun = True
    if arguments.apihost:
        config.grants.apihost = arguments.apihost

    configurator = ConfigWorker(config)

    if arguments.init:
        configurator.init_configuration()
        return

    main_config_path = '/etc/mysql/my.cnf'

    mycnf = configurator.get_mycnf()
    client = configurator.get_clientcnf()
    commands = configurator.generate_new_settings(mycnf)
    configurator.apply_new_settings(commands)
    configurator.save(main_config_path, mycnf.getvalue())
    configurator.save_client(client.getvalue())
    configurator.apply_new_settings(commands)
    commands = configurator.generate_new_settings(mycnf)
    configurator.apply_new_settings(commands)


if __name__ == '__main__':
    main()
