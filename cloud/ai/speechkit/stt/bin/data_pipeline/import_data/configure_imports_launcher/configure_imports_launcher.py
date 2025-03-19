import os

import reactor_client as r

import cloud.ai.speechkit.stt.lib.reactor as reactor_consts


def main():
    client = r.ReactorAPIClientV1(
        base_url='https://reactor.yandex-team.ru',
        token=os.getenv('REACTOR_TOKEN'),
    )

    s3_bucket(client)


def s3_bucket(client: r.ReactorAPIClientV1):
    builder = r.reaction_builders.NirvanaReactionBuilder()

    builder.set_version(2)

    builder.set_source_graph(flow_id='11a11f61-1bb1-411f-9521-220d62526ba8')
    builder.set_target_graph(flow_id='135e44e0-b87e-40b8-88e1-4f448ff17de8')

    builder.set_reaction_path('/cloud/ai/speechkit/stt/import/s3_bucket', 'S3 bucket')

    builder.set_project(reactor_consts.project_identifier)
    builder.set_owner('o-gulyaev')

    operation = 'operation-4eadb0f3-c346-49f2-85c5-072e900cc181'

    builder.set_block_param_to_expression_var(
        operation, 'tag', r.r_objs.ExpressionVariable(var_name='tag'),
    )
    builder.set_block_param_to_expression_var(
        operation, 'source-bucket', r.r_objs.ExpressionVariable(var_name='source_bucket'),
    )
    builder.set_block_param_to_expression_var(
        operation, 'sample-size', r.r_objs.ExpressionVariable(var_name='sample_size'),
    )
    builder.set_block_param_to_expression_var(
        operation, 'source-aws-access-key-id', r.r_objs.ExpressionVariable(var_name='source_aws_access_key_id'),
    )
    builder.set_block_param_to_expression_var(
        operation, 'source-aws-secret-access-key', r.r_objs.ExpressionVariable(var_name='source_aws_secret_access_key'),
    )

    def generate_trigger(
        tag: str,
        source_bucket: str,
        sample_size: int,
        source_aws_access_key_id: str,
        source_aws_secret_access_key: str,
        cron_expression: str,
    ) -> r.r_objs.DynamicTrigger:
        return r.r_objs.DynamicTrigger(
            trigger_name=tag.replace('-', '_'),
            expression=r.r_objs.Expression('\n'.join([
                f'global tag = "{tag}";',
                f'global source_bucket = "{source_bucket}";',
                f'global sample_size = {sample_size};',
                f'global source_aws_access_key_id = "{source_aws_access_key_id}";',
                f'global source_aws_secret_access_key = Datum.nirvanaSecret("{source_aws_secret_access_key}");',
            ])),
            cron_trigger=r.r_objs.CronTrigger(cron_expression=cron_expression),
        )

    triggers = [
        generate_trigger(
            tag='taxi-p2p',
            source_bucket='talks',
            sample_size=20000,
            source_aws_access_key_id='MKnpto1wIefikd84QgEd',
            source_aws_secret_access_key='cloud-ai-taxi-p2p-talks-aws-secret-access-key',
            cron_expression='0 0 6 ? * SAT',
        ),
        generate_trigger(
            tag='khural',
            source_bucket='khuralaudio',
            sample_size=0,
            source_aws_access_key_id='W6VPflgEThIUgKqnDRvb',
            source_aws_secret_access_key='cloud-ai-mds-comserv-aws-secret-access-key',
            cron_expression='0 0 16 ? * FRI',
        ),
    ]

    builder.set_dynamic_triggers(triggers)

    client.reaction.create(builder.operation_descriptor, create_if_not_exist=True)
