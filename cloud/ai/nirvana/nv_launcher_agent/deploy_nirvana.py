import json
from argparse import ArgumentParser

from cloud.ai.nirvana.nv_launcher_agent.lib.release_creator import push_new_release
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class DeployService:
    @staticmethod
    def get_options(parser: ArgumentParser):
        parser.add_argument('--release-name', type=str, required=True)
        parser.add_argument('--revision-id', type=str, required=True)
        parser.add_argument('--message', type=str, required=True)
        parser.add_argument('--aws-access-key', type=str, required=True)
        parser.add_argument('--aws-secret-key', type=str, required=True)
        parser.add_argument('--source-dir', type=str, required=True)
        return parser

    def __init__(self, args):
        ThreadLogger.info(json.dumps({
            'release_name': args.release_name,
            'revision_id': args.revision_id,
            'message': args.message
        }, indent=4))

        push_new_release(
            release_name=args.release_name,
            revision_id=args.revision_id,
            message=args.message,
            aws_access_key=args.aws_access_key,
            aws_secret_key=args.aws_secret_key,
            source_dir=args.source_dir
        )
