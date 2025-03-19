from time import sleep

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.function_process import FunctionProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.random_name import generate_name
from cloud.ai.nirvana.nv_launcher_agent.docker_client import DockerClient
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


class DockerCacheService:
    def __init__(self, repo_prefix, key_string):
        self.docker_client = DockerClient(repo_prefix, key_string)
        self.key_string = key_string

    def upload_layer(self, local_name, porto_layers_hash, tag='latest'):
        remote_image_name = generate_name() + "-" + porto_layers_hash[:4]

        ThreadLogger.info(f'Tagging {tag} {remote_image_name}:{tag}')
        self.docker_client.tag(local_name, remote_image_name, tag)

        ThreadLogger.info(f'Pushing to repo {remote_image_name}')
        self.docker_client.push(remote_image_name, tag)

        return remote_image_name

    def download_layer(self, remote_image_name):
        ThreadLogger.info('Running download_layer')
        self.docker_client.pull(remote_image_name)
        ThreadLogger.info('Docker pull finished')


if __name__ == '__main__':
    def run_download_image(image_name):
        from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
        service = DockerCacheService(Config.get_cr_layer_repo(), 'key.json')
        service.download_layer(image_name)

    proc = FunctionProcess("download_image", args=("cr.yandex/crppns4pq490jrka0sth/cloud-nirvana-images-cache/surly-buff-snake-53ab:latest",), target=run_download_image)
    proc.start()
    print(proc.status)
    while proc.status == ProcessStatus.RUNNING:
        sleep(5)
        continue

    print(proc.status)
    print("END")
