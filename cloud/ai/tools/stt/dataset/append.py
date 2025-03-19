import binascii
import json
import os
import os.path
import sys
import time
import wave

import boto3
from mutagen.mp3 import MP3


def crc32_from_file(filename):
    buf = open(filename, 'rb').read()
    buf = (binascii.crc32(buf) & 0xFFFFFFFF)
    return buf


def upload_file(file_name, cloud_file_name):
    session = boto3.session.Session()
    client = session.client(
        service_name='s3',
        endpoint_url='https://storage.yandexcloud.net'
    )
    client.upload_file(file_name, "audio-dataset", cloud_file_name)


# tags: set_yes_no
# source: company_name/staff login
def append_file(file_name, tags, texts, source):
    ext = file_name[-4:]
    if ext == ".wav":
        with wave.open(file_name, "r") as f:
            frames_count = f.getnframes()
            sample_rate = f.getframerate()
            duration_ms = int(frames_count / sample_rate * 1000)
            d = f.readframes(frames_count)
            id = binascii.crc32(d)
    else:
        id = crc32_from_file(file_name)
        f = MP3(file_name)
        sample_rate = f.info.sample_rate
        duration_ms = int(1000 * f.info.length)

    upload_file(file_name, "%d" % id + ext)

    url = "s3://audio-dataset/%d" % id + ext
    structure = {
        "id": id,
        "tags": tags,
        "texts": texts,
        "source": source,
        "duration_ms": duration_ms,
        "sample_rate": sample_rate,
        "url": url,
        "created": int(time.time()),
    }

    with open(os.path.join(os.path.dirname(sys.argv[0]), "storage.txt"), "a") as f:
        f.write(json.dumps(structure, ensure_ascii=False) + "\n")
    return url


def append_dir(path, tags, source):
    for file in os.listdir(path):
        total_file_name = "%s/%s" % (path, file)
        if file[-4:] != ".wav" and file[-4:] != ".mp3":
            continue
        id = file[:-4]
        texts = []
        with open("%s/%s.txt" % (path, id), "r") as f:
            while True:
                r = f.readline()
                if r == "":
                    break

                texts.append(r.replace("\n", ""))
        append_file(total_file_name, tags, texts, source)


def download_files(output_dir, rows_ids):
    with open(os.path.join(os.path.dirname(sys.argv[0]), "storage.txt"), "r") as f:
        data = f.readlines()
    for i in rows_ids:
        audio_info = json.loads(data[i])
        with open(os.path.join(output_dir, audio_info["id"] + ".txt"), "w") as f:
            if len(audio_info["texts"]) > 0:
                f.write(audio_info["texts"][0])
            else:
                f.write("")
        session = boto3.session.Session()
        client = session.client(
            service_name='s3',
            endpoint_url='https://storage.yandexcloud.net'
        )
        path_parts = audio_info["url"].split("/")
        get_object_response = client.get_object(Bucket='audio-dataset', Key=path_parts[-1])
        with open(os.path.join(output_dir, path_parts[-1]), "wb") as f:
            f.write(get_object_response['Body'].read())
