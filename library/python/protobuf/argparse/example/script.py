import library.python.protobuf.argparse.example.conf_pb2 as conf_pb2
from library.python.protobuf.argparse import ArgumentParser


def main():
    p = ArgumentParser(conf_pb2.TScriptArgs)
    p.add_argument('--extra-argument', dest='extra_argument')
    args, conf = p.parse_args()
    print(args.extra_argument)
    print('protobuf field ui32_field', conf.ui32_field)
    print('protobuf field repeated_string_field', conf.repeated_string_field)
