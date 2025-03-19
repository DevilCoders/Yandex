import json
import numpy as np

from argparse import ArgumentParser
from pathlib import Path


def update_speaker_to_model_map(config_path: Path, speaker_whitelist: set):
    config = json.loads(config_path.read_text())
    speaker_to_model_map = {}
    for speaker, model in config['speaker_to_model'].items():
        if speaker in speaker_whitelist:
            speaker_to_model_map[speaker] = model
    config['speaker_to_model'] = speaker_to_model_map
    config_path.write_text(json.dumps(config, ensure_ascii=False, indent=4))


def update_acoustic_resources_config(config_path: Path, speaker_whitelist: set):
    config = json.loads(config_path.read_bytes())

    id_whitelist = set()
    speakers = {}

    for speaker_name, speaker_desc in config['speakers'].items():
        if speaker_name in speaker_whitelist:
            id_whitelist.add(speaker_desc['id'])
            speakers[speaker_name] = speaker_desc

    embeddings_path = config_path.parent / config['embeddings']
    embeddings = dict(np.load(embeddings_path))
    for _, speaker_desc in config['speakers'].items():
        speaker_id = speaker_desc['id']
        if speaker_id not in id_whitelist:
            embeddings['speaker_embeddings'][speaker_id, :] = np.inf

    config['speakers'] = speakers
    config_path.write_text(json.dumps(config, ensure_ascii=False, indent=4))

    np.savez(embeddings_path, **embeddings)
    print(f'{config_path}: {len(id_whitelist)} embeddings kept')


def remove_speakers(lingware_path: str, speaker_whitelist: list):
    speaker_whitelist = set(speaker_whitelist)
    for path in Path(lingware_path).rglob('*'):
        if path.name == 'acoustic_resources_config.json':
            update_acoustic_resources_config(path, speaker_whitelist)
        elif path.name == 'tts_server_config.json':
            update_speaker_to_model_map(path, speaker_whitelist)


def main():
    parser = ArgumentParser()
    parser.add_argument('--lingware-path', required=True, help='path to tts lingware')
    parser.add_argument('--speaker-whitelist', nargs='+', required=True, help='list of speakers to be kept')
    args = parser.parse_args()
    remove_speakers(args.lingware_path, args.speaker_whitelist)


if __name__ == '__main__':
    main()
