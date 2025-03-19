#!/usr/bin/env python3
import argparse
import os
import time
import sys
import socket

from yc_common.clients.teamcity import TeamcityClient, TeamcityError

DELETE_RETRIES = 10


def environ_or_required(key):
    return (
        {'default': os.environ.get(key)} if os.environ.get(key)
        else {'required': True}
    )


def _check_active(tc_client, agent_name):
    resp = tc_client.get_agent(agent_name)
    if resp['enabled'] or resp.get('build'):
        print('{} will be disabled'.format(agent_name))
        return True
    else:
        print('{} already disabled and free'.format(agent_name))
        return False


def _exit(message, code):
    print(message)
    sys.exit(code)


def main(args):
    all_agents_by_host = set()
    disabled_agents = set()
    deleted_agents = set()
    tc_client = TeamcityClient('https://teamcity.yandex-team.ru', None, None, args.token)
    resp = tc_client.get_agents_in_pool(args.pool_id)
    for agent in resp['agent']:
        agent_name = agent['name']
        if agent_name.startswith(args.fqdn):
            all_agents_by_host.add(agent_name)
            if not _check_active(tc_client, agent_name):
                disabled_agents.add(agent_name)

    for agent in (all_agents_by_host - disabled_agents):
        tc_client.disable_agent(agent, comment=args.ticket_id)

    # wait for agents will be free from active builds
    while True:
        for agent in (all_agents_by_host - disabled_agents):
            resp = tc_client.get_agent(agent)
            if resp.get('build'):
                print('{} is busy for {}'.format(agent, resp['build']))
                continue
            disabled_agents.add(agent)
        if all_agents_by_host == disabled_agents:
            print('All agents were disabled and free from builds')
            break
        time.sleep(60)

    if args.delete:
        for i in range(0, DELETE_RETRIES):
            for agent in disabled_agents:
                try:
                    resp = tc_client.get_agent(agent)
                except TeamcityError:
                    print('Agent was deleted already')
                    continue
                if not resp['connected']:
                    try:
                        tc_client.delete_agent(agent)
                        deleted_agents.add(agent)
                    except TeamcityError:
                        continue
                else:
                    print(resp)
                    print('Could not delete agent from teamcity before shutdown agent first')
            if deleted_agents == disabled_agents:
                _exit('All disabled agents were deleted', 0)
            time.sleep(60)
        _exit('Could not retry anymore', 1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--fqdn')
    parser.add_argument('--token', **environ_or_required('TEAMCITY_TOKEN'))
    parser.add_argument('--pool-id', required=True)
    parser.add_argument('--ticket-id', required=True)
    parser.add_argument('--delete', default=False, action='store_true')
    parser.add_argument('--delete-retries', default=DELETE_RETRIES)
    args = parser.parse_args()
    if not args.token:
        _exit('You have to set TEAMCITY_TOKEN', 1)
    if args.fqdn == socket.gethostname():
        _exit('Cannot disable agent where build running', 1)
    main(args)
