import nirvana.job_context as nv
import yt.wrapper as yt
import os
import json
import speechkit
from yt_reader import YtReader
import tarfile


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


def make_dataset_from_yt(folder_ref, folder_syn, yt_set_config, honeypots):
    speaker = yt_set_config['speaker']
    yt_table = yt_set_config['yt_path']
    id_col = yt_set_config['uuid_column']
    text_col = yt_set_config['text_column']
    limit = yt_set_config['limit']

    os.mkdir(folder_ref)
    os.mkdir(folder_syn)

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

        ref_path = f'./{folder_ref}/' + cur_id + '_ref.wav'
        syn_path = f'./{folder_syn}/' + cur_id + '_syn.wav'

        if honeypots:
            synthesize(ref_path, text, speaker, 'prod')
            processed_row = {'uuid': cur_id, 'reference': ref_path, 'text': text, 'synthesis': ref_path, 'speaker': speaker}
        else:
            synthesize(ref_path, text, speaker, 'prod')
            synthesize(syn_path, text, speaker, 'custom')
            processed_row = {'uuid': cur_id, 'reference': ref_path, 'text': text, 'synthesis': syn_path, 'speaker': speaker}

        dataset.append(processed_row)
        count += 1

    return dataset


markup_name = './tasks-dataset.json'
honeypots_name = './control-tasks-dataset.json'
markup_folder = 'tasks_folder'
honeypots_folder = 'control_tasks_folder'

os.mkdir(markup_folder)
tasks = []
for i, cur_tasks_config in enumerate(markup_params['sbs']['tasks_config']):
    res = make_dataset_from_yt(f'{markup_folder}/ref_{i}', f'{markup_folder}/syn_{i}', cur_tasks_config, False)
    tasks += res

with open(markup_name, 'w', encoding='utf-8') as f:
    json.dump(tasks, f, ensure_ascii=False, indent=4)


os.mkdir(honeypots_folder)
control_tasks = []
for i, cur_tasks_config in enumerate(markup_params['sbs']['control_tasks_config']):
    res = make_dataset_from_yt(f'{honeypots_folder}/ref_{i}', f'{honeypots_folder}/syn_{i}', cur_tasks_config, True)
    control_tasks += res

with open(honeypots_name, 'w', encoding='utf-8') as f:
    json.dump(control_tasks, f, ensure_ascii=False, indent=4)


tar = tarfile.open(outputs.get('tasks'), "w:gz")
for name in [markup_folder, markup_name]:
    tar.add(name)
tar.close()

tar = tarfile.open(outputs.get('control_tasks'), "w:gz")
for name in [honeypots_folder, honeypots_name]:
    tar.add(name)
tar.close()
