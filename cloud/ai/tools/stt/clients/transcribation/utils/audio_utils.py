import os
import sox
from sox.core import SoxiError


SUPPORTED_RATES = [8000, 16000, 48000]


def list_audio_files(path):
    if os.path.isfile(path):
        if __is_audio_file(path):
            yield path
        return

    for file in os.listdir(path):
        file_path = os.path.join(path, file)
        if __is_audio_file(file_path):
            yield file_path


def convert_for_transcribation(file):
    finfo = sox.file_info.info(file)

    if __is_OggOpus(file, finfo):
        return (file, finfo)

    file, finfo = __convert_to_pcm(file, finfo)
    return (file, finfo)


def __is_audio_file(file_path):
    try:
        sox.core.soxi(file_path, 't')
        return True
    except SoxiError as e:
        print("skipping '%s', not an audio file" % file_path)
        return False


def __convert_to_pcm(file, in_finfo):
    inp_path_base, _ = os.path.splitext(file)

    # SoX only can convert to .raw files, which is the same as .pcm.
    # Rename to .pcm after convertion just for clarity.
    raw_file = inp_path_base + ".raw"
    out_file = inp_path_base + ".pcm"

    out_rate = 16000 # just agreed to always convert to 16kHz
    out_bits = 16
    out_encoding = "signed-integer"
    out_type = "raw"
    out_channels = in_finfo["channels"]

    sox_transformer = sox.Transformer()
    sox_transformer.set_output_format(
            file_type=out_type,
            rate=out_rate,
            bits=out_bits,
            encoding=out_encoding,
            channels=out_channels)
    res = sox_transformer.build(file, raw_file)

    if not res:
        raise "Failed to convert file '%s' to LPCM" % file

    os.rename(raw_file, out_file)

    finfo = {
            "sample_rate": out_rate,
            "bitdepth": out_bits,
            "encoding": out_encoding,
            "channels": out_channels}
    return (out_file, finfo)


def __is_OggOpus(file, finfo):
    _, ext = os.path.splitext(file)
    return (ext in [".ogg", ".opus"]) and ("opus" in finfo["encoding"].lower())
