import os
import sys
import json
from http.server import BaseHTTPRequestHandler, HTTPServer

from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.tasks.tools.configs_api import get_configs_from_api as get_tvm_request


TVM_ID = int(os.getenv("TVM_ID", "2001021"))  # use cryprox tvm_id
TVM_SECRET = os.getenv("TVM_SECRET")
CONFIGS_API_TVM_ID = int(os.getenv("CONFIGSAPI_TVM_ID", "2000627"))
CONFIGS_API_HOST = os.getenv("CONFIGS_API_HOST", "preprod.aabadmin.yandex.ru")
PORT = int(os.getenv("CONFIG_STUB_PORT", "5000"))


if __name__ == "__main__":
    configs_api_url = f"https://{CONFIGS_API_HOST}/v2/configs_hierarchical_handler"

    tvm_settings = TvmApiClientSettings(
        self_tvm_id=TVM_ID,
        self_secret=TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGS_API_TVM_ID)
    )

    tvm_client = TvmClient(tvm_settings)

    def get_configs(params=None):
        sys.stderr.write(f"Get configs from {configs_api_url}\n")
        return json.dumps(get_tvm_request(configs_api_url, tvm_client, params))

    class HttpProcessor(BaseHTTPRequestHandler):

        def do_GET(self):

            self.send_response(200)
            self.send_header('content-type', 'application/json')
            self.end_headers()
            self.wfile.write(configs.encode('utf-8'))

        def do_POST(self):
            global configs
            self.send_response(200)
            self.end_headers()
            length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(length)
            sys.stderr.write(f"POST data: {post_data}\n")
            params = {'replace_configs': post_data.decode('utf-8')}
            configs = get_configs(params=params)

    configs = get_configs()
    sys.stderr.write(f'Start stub server on {PORT} port\n')
    server = HTTPServer(("localhost", PORT), HttpProcessor)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.shutdown()

    sys.stderr.write('Stop stub server\n')
