import os
import time

import json
import urllib


def transcribe(uri, file, finfo, args):
    request_id = __send_on_transcribtion(uri, file, finfo, args)
    res = __wait_for_result(request_id, args.api_key)
    return res


def get_result(request_id, api_key):
    url = urllib.request.Request("https://operation.api.cloud.yandex.net/operations/" + request_id)
    url.add_header("Authorization", "Api-Key " + api_key)
    response = urllib.request.urlopen(url).read().decode("UTF-8")
    res_json = json.loads(response)
    if not res_json["done"]:
        return None
    else:
        return res_json["response"]


def text_from_response(response, file):
    if "chunks" not in response:
        print("No chunks were returned for file " + file)
        return ""

    texts = []
    for chunk in response["chunks"]:
        texts.append(chunk["alternatives"][0]["text"])
    return " ".join(texts)


def __wait_for_result(request_id, api_key):
    res = get_result(request_id, api_key)
    while res is None:
        time.sleep(0.5)
        res = get_result(request_id, api_key)
    return res


def __send_on_transcribtion(uri, file, finfo, args):
    configs = __form_configs(file, finfo, args, uri)
    url = urllib.request.Request("https://transcribe.api.cloud.yandex.net/speech/stt/v2/longRunningRecognize", data=configs.encode("utf-8"))
    url.add_header("Authorization", "Api-Key " + args.api_key)
    response = urllib.request.urlopen(url).read().decode("UTF-8")
    response_json = json.loads(response)
    request_id = response_json["id"]
    return request_id


def __form_configs(file, finfo, args, uri):
    _, ext = os.path.splitext(file)
    if ext in [".ogg", ".opus"]:
        return __form_configs_OggOpus(args, uri)
    else:
        return __form_configs_pcm(finfo, args, uri)


def __form_configs_pcm(finfo, args, uri):
    return u"""{{
    "config": {{
        "specification": {{
            "languageCode": "{}",
            "audioEncoding": "{}",
            "sampleRateHertz": {},
            "audioChannelCount": {},
            "raw_results": {}
        }}
    }},
    "audio": {{
        "uri": "{}"
    }}
}}""".format(args.lang, "LINEAR16_PCM", int(finfo["sample_rate"]), finfo["channels"], str(args.raw_results).lower(), uri)


def __form_configs_OggOpus(args, uri):
    return u"""{{
    "config": {{
        "specification": {{
            "languageCode": "{}",
            "raw_results": {}
        }}
    }},
    "audio": {{
        "uri": "{}"
    }}
}}""".format(args.lang, str(args.raw_results).lower(), uri)
