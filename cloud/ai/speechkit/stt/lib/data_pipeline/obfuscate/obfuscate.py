import math
import os
import random

from pydub import AudioSegment

from cloud.ai.speechkit.stt.lib.data_pipeline.files import bits_dir, obfuscated_bits_dir

convert_cmd = 'sox {old_filename} {new_filename} --norm pitch {pitch}'


def convert_file(filename: str, pitch: int):
    cmd = convert_cmd.format(
        old_filename=os.path.join(bits_dir, filename),
        new_filename=os.path.join(obfuscated_bits_dir, filename),
        pitch=pitch,
    )
    print(cmd)
    exit_code = os.system(cmd)
    if exit_code != 0:
        raise RuntimeError(f'Command "{cmd}" finished with non-zero code {exit_code}')


reduce_volume_cmd = 'sox -v 0.25 {old_filename} {new_filename}'
tmp_audio_dir = 'tmp_audio'


def reduce_volume(filename: str):
    cmd = reduce_volume_cmd.format(
        old_filename=os.path.join(tmp_audio_dir, filename),
        new_filename=os.path.join(obfuscated_bits_dir, filename),
    )
    print(cmd)
    exit_code = os.system(cmd)
    if exit_code != 0:
        raise RuntimeError(f'Command "{cmd}" finished with non-zero code {exit_code}')


# [-max_pitch, -min_pitch] or [+min_pitch, +max_pitch],
def generate_pitch(min_pitch: int, max_pitch: int) -> int:
    pitch = random.randrange(min_pitch, max_pitch + 1)
    if random.choice([True, False]):
        pitch = -pitch
    return pitch


min_dbfs = -99.0


def get_audio_file_dbfs(filename: str) -> (str, float):
    dbfs = AudioSegment.from_wav(filename).dBFS
    if dbfs == -math.inf:
        print(f'{filename} has -inf dBFS, replace it with {min_dbfs}')
        dbfs = min_dbfs
    return filename, dbfs


def volume_reduction_needed(dbfs_before: float, dbfs_after: float) -> bool:
    if dbfs_after < dbfs_before:
        return False
    if dbfs_after == 0:
        return True
    # dBFS values are negative
    return (dbfs_before / dbfs_after) > 4.0 and (dbfs_after - dbfs_before) > 30.0
