#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import argparse
import BaseHTTPServer
import jsonschema
import json


def ParseArgs():
    parser = argparse.ArgumentParser(description='Mock for nanny.yandex-team.ru')
    parser.add_argument('--port', type=int)
    parser.add_argument('--schema', type=str)

    return parser.parse_args()


class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path.rstrip('/') == '/v1/requests/sandbox_releases':
            try:
                contentLength = int(self.headers['Content-Length'])

                requestBody = json.loads(self.rfile.read(contentLength))
                jsonschema.validate(requestBody, server.ReleaseRequestSchema)

                self.send_response(200)
                self.end_headers()

                responseBody = {
                    "id": 'SANDBOX_RELEASE-%s-%s' % (requestBody['task_id'], requestBody['release_status']),
                }
                self.wfile.write(json.dumps(responseBody))
            except Exception, ex:
                self.send_error(500, message=str(ex))
                self.end_headers()
        else:
            self.send_error(404, message=self.path)
            self.end_headers()


if __name__ == "__main__":
    try:
        args = ParseArgs()
        baseDir = os.path.dirname(os.path.realpath(__file__))

        with open(args.schema, 'rt') as file:
            schema = file.read()

        server = BaseHTTPServer.HTTPServer(("", args.port), RequestHandler)
        server.ReleaseRequestSchema = json.loads(schema)
        server.serve_forever()

    except KeyboardInterrupt:
        print "^C received, shutting down server"
        server.socket.close()
