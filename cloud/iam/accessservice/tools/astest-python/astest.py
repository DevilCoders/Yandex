import argparse
import datetime
import grpc
import os
import time
import uuid

from yc_as_client import YCAccessServiceClient, entities


def randomize_id(id):
    return id.replace('*', uuid.uuid4().hex)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--certpath', default='/etc/ssl/certs/ca-certificates.crt')
    parser.add_argument('-d', '--delay', default=1.0, type=float)
    parser.add_argument('-e', '--endpoint', default='as.private-api.cloud-preprod.yandex.net:4286')
    parser.add_argument('-p', '--permission', default='resource-manager.folders.get')
    parser.add_argument('-r', '--rps', default=100, type=int)
    parser.add_argument('--plaintext', default=False, action='store_true')
    parser.add_argument('--ssl-target-name', default=None)
    parser.add_argument('-t', '--token', default=os.getenv('IAM_TOKEN', default='TOKEN'))
    parser.add_argument('-v', '--verbose', default=False, action='store_true')
    parser.add_argument('resource', nargs='?')
    args = parser.parse_args()

    print("Access service client sarted. Resource {}, endpoint {}, rps {}, delay {} sec.".format(
        args.resource,
        args.endpoint,
        args.rps,
        args.delay,
    ))

    with open(args.certpath, "rb") as f:
        certs = f.read()

    if args.resource:
        [resource_type, resource_id] = args.resource.split(':')
    else:
        [resource_type, resource_id] = [None, None]

    if args.plaintext:
        channel = grpc.insecure_channel(args.endpoint)
    else:
        if args.ssl_target_name:
            grpc_options = (('grpc.ssl_target_name_override', args.ssl_target_name),)
        else:
            grpc_options = ()
        credentials = grpc.ssl_channel_credentials(root_certificates=certs)
        channel = grpc.secure_channel(args.endpoint, credentials, options=grpc_options)

    client = YCAccessServiceClient(channel)

    while True:
        max_duration = 0
        min_duration = 999999999
        sum_duration = 0
        count = 0
        err_count = 0
        start = time.time()
        fs = []
        for i in range(args.rps):
            if resource_id:
                fs.append(client.authorize.future(
                    args.permission,
                    [
                        entities.Resource(randomize_id(resource_id), resource_type),
                    ],
                    iam_token=args.token
                ))
            else:
                fs.append(client.authenticate.future(iam_token=args.token))

        results = {}
        while len(fs) > 0:
            new = []
            for f in fs:
                if f.running():
                    new.append(f)
                else:
                    count += 1
                    duration = time.time() - start
                    if duration < min_duration:
                        min_duration = duration
                    if duration > max_duration:
                        max_duration = duration
                    sum_duration += duration

                    try:
                        f.result()
                        results["OK"] = 1 + results.get("OK", 0)
                    except Exception as e:
                        if args.verbose:
                            print("Auth error: '{!r}'!".format(e))
                        results[e.message] = 1 + results.get(e.message, 0)
                        err_count += 1
            fs = new
        print("{}", results)
        print("{}: count {}, errors {}, min {}, max {}, avg {}".format(
            str(datetime.datetime.now()),
            count,
            err_count,
            min_duration,
            max_duration,
            sum_duration / count,
            ))
        total = time.time() - start
        if total < args.delay:
            time.sleep(args.delay - total)


if __name__ == '__main__':
    main()
