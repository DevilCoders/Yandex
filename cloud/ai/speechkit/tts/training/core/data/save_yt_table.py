import sys
from os.path import abspath, dirname

# enabling modules discovery from global entrypoint
sys.path.append(abspath(dirname(__file__) + '/../'))

import argparse
import io
import os
import pickle

import numpy as np
from tqdm import tqdm
import yt.wrapper as yt

from data import RawTtsSample


def decode_bytes(data):
    if isinstance(data, bytes):
        return data.decode()
    if isinstance(data, dict):
        return dict(map(decode_bytes, data.items()))
    if isinstance(data, tuple):
        return tuple(map(decode_bytes, data))
    if isinstance(data, list):
        return list(map(decode_bytes, data))
    return data


def save_yt_table(table_path: str, save_path: str):
    name = table_path.split("/")[-1]
    save_path = os.path.join(save_path, name)
    os.makedirs(save_path, exist_ok=True)
    offset = 0
    total_samples = yt.row_count(table_path)
    print(table_path)
    with open(f"{save_path}/table", "wb") as table_f, open(f"{save_path}/meta", "w") as meta_f:
        meta_f.write(f"{total_samples}\n")
        for row in tqdm(yt.read_table(yt.TablePath(table_path), yt.YsonFormat(encoding=None))):
            try:
                sample_id = row[b"ID"].decode()
                utterance = decode_bytes(row[b"utterance"])
                speaker = row[b"speaker"].decode()
                text = row[b"text"].decode()

                accented_text = row.get(b"accented_text")
                accented_text = accented_text.decode() if accented_text is not None else None

                template_text = row.get(b"template_text")
                template_text = template_text.decode() if template_text is not None else None

                text_variables = row.get(b"text_variables")
                text_variables = decode_bytes(text_variables) if text_variables is not None else None

                audio_variables = row.get(b"audio_variables")
                audio_variables = decode_bytes(audio_variables) if audio_variables is not None else None

                wav = row[b"pcm__wav"]
                mel = np.load(io.BytesIO(row[b"mel__npy"]))
                pitch = np.load(io.BytesIO(row[b"pitch__npy"]))

                sample = RawTtsSample(
                    id=sample_id,
                    lang="ru",
                    utterance=utterance,
                    speaker=speaker,
                    text=text,
                    accented_text=accented_text,
                    template_text=template_text,
                    text_variables=text_variables,
                    audio_variables=audio_variables,
                    wav=wav,
                    mel=mel,
                    pitch=pitch
                )
            except Exception as e:
                print(e)
                meta_f.write(f"0 {offset}\n")
                continue

            sample_bytes = io.BytesIO()
            pickle.dump(sample, sample_bytes)
            sample_bytes.seek(0)
            sample_bytes = sample_bytes.read()

            table_f.write(sample_bytes)
            meta_f.write(f"{len(sample_bytes)} {offset}\n")
            offset += len(sample_bytes)
            total_samples += 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--table", type=str, required=True)
    parser.add_argument("--path", type=str, required=True)
    parser.add_argument("--threads", type=int, default=24)
    args = parser.parse_args()

    yt.config["proxy"]["url"] = "hahn"
    yt.config["read_parallel"]["enable"] = True
    yt.config["read_parallel"]["max_thread_count"] = args.threads

    save_yt_table(args.table, args.path)
