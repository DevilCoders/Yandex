import os
from io import BytesIO

import yt.wrapper as yt

from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger


def set_yt_config(yt_config, yt_token, proxy):
    yt_config['token'] = yt_token
    yt_config['proxy']['url'] = proxy


class LayerManager:
    def __init__(self, layers_dir):
        ThreadLogger.info(f'Init LayerManager(layers_dir={layers_dir})')
        self.layers_dir = layers_dir
        if not os.path.exists(self.layers_dir):
            os.makedirs(self.layers_dir)

        self.layers = {}  # yt_path -> local_path
        # TODO: load already downloaded layers

    @staticmethod
    def __save_layer_to_disk(stream: BytesIO, dst_path):
        with open(dst_path, 'wb') as f:
            block = stream.read()
            while len(block) != 0:
                f.write(block)
                block = stream.read()

    def get_layer_by_yt_path(self, yt_path, yt_token, proxy='hahn'):
        ThreadLogger.info(f'Get layer by YtPath yt_path={yt_path}')
        set_yt_config(yt.config, yt_token, proxy)

        if yt_path in self.layers:
            return self.layers[yt_path]

        layer_name = os.path.basename(yt_path)
        local_layer_path = os.path.join(self.layers_dir, layer_name)

        if os.path.exists(local_layer_path):
            ThreadLogger.info(f'Return existing from disk yt_path={yt_path}, local_path={local_layer_path}')
            self.layers[yt_path] = local_layer_path
            return local_layer_path

        ThreadLogger.info(f'Save layer to disk yt_path={yt_path}, local_path={local_layer_path}')
        LayerManager.__save_layer_to_disk(
            yt.read_file(yt_path),
            local_layer_path
        )
        self.layers[yt_path] = local_layer_path
        return local_layer_path
