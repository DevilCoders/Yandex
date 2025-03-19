from argparse import ArgumentParser

from cloud.ai.nirvana.nv_launcher_agent.agent import JobAgentService
from cloud.ai.nirvana.nv_launcher_agent.proxy import ProxyService
from cloud.ai.nirvana.nv_launcher_agent.deploy_nirvana import DeployService
from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


def main():
    parser = ArgumentParser()
    parser.add_argument('--project-dir')

    subparsers = parser.add_subparsers(
        help='--proxy to run proxy, --agent to run agent',
        required=True,
        dest='mode'
    )

    proxy_parser = subparsers.add_parser('proxy', help='run proxy')
    ProxyService.get_options(proxy_parser)

    agent_parser = subparsers.add_parser('agent', help='run agent')
    JobAgentService.get_options(agent_parser)

    deploy_parser = subparsers.add_parser('deploy', help='run deploy')
    DeployService.get_options(deploy_parser)

    args = parser.parse_args()

    if args.project_dir is not None:
        Config.set_project_dir(args.project_dir)
        ThreadLogger.info(f'Current project dir={Config.get_project_dir()}')

    if args.mode == 'proxy':
        ProxyService(args)
    elif args.mode == 'agent':
        JobAgentService(args)
    elif args.mode == 'deploy':
        assert args.project_dir is None
        DeployService(args)
    else:
        raise Exception('Bad arguments')


if __name__ == '__main__':
    main()
