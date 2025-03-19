import json

from argparse import ArgumentParser
from pathlib import Path


def set_default_speaker(lingware_path: str, speaker: list):
    for path in Path(lingware_path).rglob('*'):
        if path.name != 'tts_server_config.json':
            continue

        config = json.loads(path.read_text())
        config['default_speaker'] = speaker
        path.write_text(json.dumps(config, ensure_ascii=False, indent=4))


def main():
    parser = ArgumentParser()
    parser.add_argument('--lingware-path', required=True, help='path to tts lingware')
    parser.add_argument('--speaker', required=True, help='default speaker name')
    args = parser.parse_args()
    set_default_speaker(args.lingware_path, args.speaker)


if __name__ == '__main__':
    main()
