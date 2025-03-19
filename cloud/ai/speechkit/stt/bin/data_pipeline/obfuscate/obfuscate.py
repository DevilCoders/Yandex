#!/usr/bin/python3

import os
import ujson as json
from shutil import copyfile
from multiprocessing.pool import ThreadPool

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data_pipeline.files import unpack_and_list_files, bits_dir, obfuscated_bits_dir
from cloud.ai.speechkit.stt.lib.data_pipeline.obfuscate import (
    get_audio_file_dbfs,
    generate_pitch,
    convert_file,
    tmp_audio_dir,
    reduce_volume,
    volume_reduction_needed,
)


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = nv.context().outputs
    params = nv.context().parameters

    records_bits_files_path = inputs.get('records_bits_files.tar.gz')
    obfuscated_records_bits_files_path = outputs.get('obfuscated_records_bits_files.tar.gz')
    obfuscate_data_path = outputs.get('obfuscate_data.json')
    min_pitch = params.get('min_pitch')
    max_pitch = params.get('max_pitch')

    if min_pitch == 0 and max_pitch == 0:
        pass
    else:
        assert min_pitch > 0
        assert max_pitch > 0
        assert max_pitch > min_pitch

    filenames = unpack_and_list_files(archive_path=records_bits_files_path, directory_path=bits_dir)

    pool = ThreadPool(processes=16)

    filename_to_dbfs_before = {
        os.path.basename(filename): dbfs
        for filename, dbfs in pool.map(get_audio_file_dbfs, [os.path.join(bits_dir, f) for f in filenames])
    }

    os.system(f'mkdir {obfuscated_bits_dir}')

    filename_to_pitch = {}
    convert_args = []
    for filename in filenames:
        pitch = generate_pitch(min_pitch, max_pitch)
        filename_to_pitch[filename] = pitch
        convert_args.append((filename, pitch))

    pool.starmap(convert_file, convert_args)

    filename_to_dbfs_after = {
        os.path.basename(filename): dbfs
        for filename, dbfs in pool.map(get_audio_file_dbfs, [os.path.join(obfuscated_bits_dir, f) for f in filenames])
    }

    assert filename_to_dbfs_before.keys() == filename_to_dbfs_after.keys() == filename_to_pitch.keys()

    # CLOUD-43094
    # Records with white noise become very loud after volume normalization.
    # So we save tolokers ears and decrease volume if volume diff after
    # normalization is too big.
    filenames_to_reduce_volume = []
    os.system(f'mkdir {tmp_audio_dir}')
    for filename in filenames:
        dbfs_before = filename_to_dbfs_before[filename]
        dbfs_after = filename_to_dbfs_after[filename]
        if volume_reduction_needed(dbfs_before, dbfs_after):
            filenames_to_reduce_volume.append(filename)
            copyfile(os.path.join(obfuscated_bits_dir, filename), os.path.join(tmp_audio_dir, filename))

    pool.map(reduce_volume, filenames_to_reduce_volume)

    filename_to_dbfs_after_reduced = {
        os.path.basename(filename): dbfs
        for filename, dbfs in pool.map(
            get_audio_file_dbfs, [os.path.join(obfuscated_bits_dir, f) for f in filenames_to_reduce_volume]
        )
    }

    print(f'Volume reduced for {len(filenames_to_reduce_volume) / len(filenames):.3f} fraction of records')

    bit_name_to_data = {}
    for filename in filenames:
        bit_name_to_data[os.path.splitext(filename)[0]] = {
            'pitch': filename_to_pitch[filename],
            'dbfs_before': filename_to_dbfs_before[filename],
            'dbfs_after': filename_to_dbfs_after[filename],
            'dbfs_after_reduced': filename_to_dbfs_after_reduced.get(filename),
        }

    os.system(f'tar czf {obfuscated_records_bits_files_path} {obfuscated_bits_dir}')

    with open(obfuscate_data_path, 'w') as f:
        json.dump(bit_name_to_data, f, indent=4)
