import logging
import requests
import time

from ci.tasklet.common.proto import service_pb2 as ci
from tasklet.services.yav.proto import yav_pb2 as yav

from cloud.mdb.sandbox.tasklets.teamcity.proto import run_build_pb2, run_build_tasklet


logger = logging.getLogger(__name__)


class RunBuildImpl(run_build_tasklet.RunBuildBase):
    @property
    def secret(self):
        spec = yav.YavSecretSpec(uuid=self.input.context.secret_uid, key='teamcity.aw.cloud.token')
        return self.ctx.yav.get_secret(spec).secret

    def run(self):
        progress = ci.TaskletProgress()
        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress.progress = 0
        progress.module = 'TEAMCITY'
        self.ctx.ci.UpdateProgress(progress)

        teamcity_api_url = self.input.teamcity_api.url
        headers = {
            'Authorization': f'Bearer {self.secret}',
            'Accept': 'application/json',
        }
        build_type_id = self.input.build_type.id
        props = []
        for k, v in self.input.build_type.properties.items():
            props.append({'name': k, 'value': v})
        data = {'buildType': {'id': build_type_id}, 'properties': {'property': props}}
        response = requests.post(teamcity_api_url + '/app/rest/buildQueue', headers=headers, json=data)
        logger.debug(f'{response.request.method} {response.status_code} {response.url}')
        response.raise_for_status()
        try:
            build = response.json()
            build_url = build['webUrl']
            build_href = build['href']
        except (ValueError, KeyError):
            raise Exception('Teamcity build was not started')

        progress.status = ci.TaskletProgress.Status.RUNNING
        progress.url = build_url
        progress.text = 'Build in queue'
        self.ctx.ci.UpdateProgress(progress)

        while True:
            response = requests.get(teamcity_api_url + build_href, headers=headers)
            logger.debug(f'{response.request.method} {response.status_code} {response.url}')
            response.raise_for_status()
            build = response.json()
            build_id = build['id']
            build_state = build['state']
            if build_state != 'queued':
                build_status = build['status']
                build_status_text = build['statusText']
                if build_state in 'running':
                    try:
                        progress.progress = build['percentageComplete'] / 100
                    except:
                        pass
                    progress.text = build_status_text
                    self.ctx.ci.UpdateProgress(progress)
                else:
                    self.output.build.url = build_url
                    response = requests.get(
                        f'{teamcity_api_url}/app/rest/builds/id:{build_id}/artifacts', headers=headers
                    )
                    logger.debug(f'{response.request.url} {response.url} {response.text}')
                    response.raise_for_status()
                    self.output.build_artifacts.extend(
                        run_build_pb2.BuildArtifact(
                            name=artifact['name'],
                            url=f'{teamcity_api_url}/repository/download/{build_type_id}/{build_id}:id/{artifact["name"]}',
                        )
                        for artifact in response.json()['file']
                    )
                    progress.progress = 1
                    progress.text = build_status_text
                    if build_state == 'finished' and build_status == 'SUCCESS':
                        progress.status = ci.TaskletProgress.Status.SUCCESSFUL
                        self.ctx.ci.UpdateProgress(progress)
                        return
                    else:
                        progress.status = ci.TaskletProgress.Status.FAILED
                        self.ctx.ci.UpdateProgress(progress)
                        raise Exception('Teamcity build failed')
            time.sleep(30)
