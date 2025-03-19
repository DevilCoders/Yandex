import nirvana.job_context as nv
import yt.wrapper as yt
import os
import json
import speechkit
from yt_reader import YtReader
import tarfile
import io
from scipy.io import wavfile


job_context = nv.context()
inputs = job_context.get_inputs()
outputs = job_context.get_outputs()
nv_params = job_context.get_parameters()

yt.config['proxy']['url'] = 'hahn'
yt.config['token'] = nv_params.get('yt_token')

with open(inputs.get('params'), "r") as read_file:
    markup_params = json.load(read_file)

speechkit.configure_credentials(
    aws_access_key_id = nv_params.get('temp-bucket-aws_access_key_id'),
    aws_secret_access_key = nv_params.get('temp-bucket-aws_secret_access_key'),
    aws_temp_bucket = nv_params.get('s3_temp_bucket'),
    yc_ai_token = nv_params.get('yc_ai_token')
)


def synthesize(path, text, speaker, model='prod'):
    endpoint = nv_params.get('prod_endpoint')
    use_ssl = True

    if model == 'custom':
        endpoint = nv_params.get('custom_endpoint')
        use_ssl = False

    model = speechkit.tts.SynthesisModel(endpoint=endpoint, use_ssl=use_ssl,
                                     branch_key_value=('x-service-branch', nv_params.get('branch')), voice=speaker)
    audio = model.synthesize(text)
    audio.export(path, format="wav")


def make_dataset_from_yt(folder_syn, yt_set_config):
    speaker = yt_set_config['speaker']
    yt_table = yt_set_config['yt_path']
    id_col = yt_set_config['uuid_column']
    text_col = yt_set_config['text_column']
    wav_col = yt_set_config['wav_column']
    limit = yt_set_config['limit']
    mode = yt_set_config['mode']

    os.mkdir(folder_syn)

    if mode == 'speaker':
        yt_reader = YtReader(yt_table, [id_col.encode(), text_col.encode(), wav_col.encode()])
    else:
        yt_reader = YtReader(yt_table, [id_col.encode(), text_col.encode()])

    dataset = []
    count = 0
    for yt_row in yt_reader:
        if count == limit:
            break
        if count % 100 == 0 and count > 0:
            print(f'Iteration #{count}')

        cur_id = yt_row[id_col.encode()].decode("utf-8")
        text = yt_row[text_col.encode()].decode("utf-8")

        syn_path = f'./{folder_syn}/' + cur_id + '_syn.wav'

        try:
            if mode == 'prod':
                synthesize(syn_path, text, speaker, 'prod')
            elif mode == 'custom':
                synthesize(syn_path, text, speaker, 'custom')
            elif mode == 'speaker':
                sample_rate, data = wavfile.read(io.BytesIO(yt_row[wav_col.encode()]))
                wavfile.write(syn_path, sample_rate, data)

            processed_row = {'uuid': cur_id, 'text': text, 'synthesis': syn_path, 'speaker': speaker, 'synthesis_model': mode}

            dataset.append(processed_row)

        except Exception as e:
            print('*' * 100)
            print(text, speaker)
            print(e)
            print('*' * 100)

        count += 1

    return dataset


markup_name = './tasks-dataset.json'
markup_folder = 'tasks_folder'

os.mkdir(markup_folder)
tasks = []
for i, cur_tasks_config in enumerate(markup_params['mos']['tasks_config']):
    res = make_dataset_from_yt(f'{markup_folder}/syn_{i}', cur_tasks_config)
    tasks += res

with open(markup_name, 'w', encoding='utf-8') as f:
    json.dump(tasks, f, ensure_ascii=False, indent=4)

tar = tarfile.open(outputs.get('tasks'), "w:gz")
for name in [markup_folder, markup_name]:
    tar.add(name)
tar.close()

with open(outputs.get('control_tasks'), 'w', encoding='utf-8') as f:
    json.dump({'dummy': 'empty_json'}, f, ensure_ascii=False, indent=4)
