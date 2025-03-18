import io
import json
from tempfile import NamedTemporaryFile

import click
import yaml

from tools.releaser.src.cli.utils import get_image_path, run_with_output


class DeployClient:
    def __init__(self, stage, dump_file=None):
        self.stage = stage
        self.from_dump = False

        # TODO: do this on on demand, not on instantiation.
        if dump_file:
            self.from_dump = True
            with open(dump_file, 'r') as dump:
                self.stage_data = yaml.load(dump)
        else:
            output = io.StringIO()
            self.dctl('get', 'stage', self.stage, _out=output)
            self.stage_data = yaml.load(output.getvalue(), Loader=yaml.SafeLoader)
            output.close()

        self._updated_deploy_units = []

    def dump(self):
        return yaml.dump(self.stage_data)

    def dctl(self, *args, **kwargs):
        return run_with_output('ya', 'tool', 'dctl', *args, **kwargs)

    def yp(self, *args, **kwargs):
        return run_with_output('ya', 'tool', 'yp', *args, **kwargs)

    def yp_query(self, *args, **kwargs):
        args += ('--no-tabular', '--format=json')
        kwargs.update(_out=None)
        res = self.yp(*args, **kwargs)
        res_data = res.stdout
        return json.loads(res_data)

    def update_image(self, deploy_unit, box, image, tag):
        deploy_units = self.stage_data['spec']['deploy_units']
        if deploy_unit not in deploy_units:
            return

        deploy_unit_data = deploy_units[deploy_unit]['images_for_boxes']

        if box not in deploy_unit_data:
            return

        for_update = deploy_unit_data[box]
        if for_update['name'] == get_image_path(image) and for_update['tag'] == tag:
            click.echo(
                'WARNING: Y.Deploy doesn\'t support redeploy image with same tag',
                err=True,
            )
            return

        deploy_unit_data[box].update({
            'name': get_image_path(image),
            'tag': tag,
        })
        self._updated_deploy_units.append(deploy_unit)

    def add_comment(self, deploy_comment):
        self.stage_data['spec']['revision_info'] = {'description': deploy_comment}

    def deploy(self, draft=False):
        environment_url = 'https://deploy.yandex-team.ru/stages/{}/'.format(self.stage)
        deploy_units_str = ','.join(self._updated_deploy_units)

        stage_dump_file = NamedTemporaryFile(mode='w+')
        stage_dump_file.write(self.dump())
        stage_dump_file.flush()

        if self.from_dump:
            self.dctl('copy', 'stage', stage_dump_file.name, self.stage)
        else:
            deploy_command = 'put'
            if draft:
                deploy_command = 'publish-draft'
            self.dctl(deploy_command, 'stage', stage_dump_file.name)

        stage_dump_file.close()

        if not draft:
            click.echo(click.style('Deploy {}.({}) in progress!'.format(self.stage, deploy_units_str), fg='green'))
        click.echo(click.style(environment_url, underline=True))

    def delete(self):
        self.dctl('remove', 'stage', self.stage)

    def list_hosts(self):
        """
        List the ydeploy boxes and workload ssh addresses for the current stage.

        Per-DC iteration logic discussion:
        https://st.yandex-team.ru/YPSUPPORT-420
        """
        stage = self.stage
        deploy_units_resp = self.yp_query(
            'get-object',
            '--address=xdc',
            'stage', stage,
            '/status/deploy_units',
        )
        dcs = sorted(set(
            dc
            for deploy_unit_resp in deploy_units_resp
            for deploy_unit in deploy_unit_resp.values()
            for dc in deploy_unit['multi_cluster_replica_set']['cluster_statuses']
        ))
        result = []
        for dc in dcs:
            # The recommended method for this is Service Discovery
            # See also `ya tool dctl list endpoint`, which, unfortunately, doesn't have `--format=`.
            units = self.yp_query(
                'select-objects',
                '--address={}'.format(dc),
                'pod',
                # TODO: this probably needs updating to support deploy_unit / box / workload names divergence.
                '/labels/deploy/deploy_unit_id',
                '/status/dns/persistent_fqdn',
                "--filter=[/labels/deploy/stage_id]='{}'".format(stage),
            )
            result.extend(
                dict(
                    dc=dc,
                    deploy_unit=deploy_unit,
                    fqdn=fqdn,
                    url='root@{}.{}'.format(deploy_unit, fqdn),
                )
                for deploy_unit, fqdn in units
            )
        return result
