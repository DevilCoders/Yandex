import sys
import json
import random
import tarfile
from uuid import uuid4
from pathlib import Path
from typing import Optional
from collections import defaultdict

from nope import JobProcessorOperation
from nope.endpoints import *
from nope.parameters import *

import yt.wrapper as yt

from text_templates import check_templates, generate_text
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


class ImportTemplates(JobProcessorOperation):
    name = 'Voice recorder templates to YT'
    owner = 'cloud-ml'
    description = 'Upload templates for voice recorder to YT'

    class Parameters(JobProcessorOperation.Parameters):
        dataset_name = StringParameter(nirvana_name='dataset-name', required=True)
        folder_id = StringParameter(nirvana_name='folder-id', required=True)
        templates_root_dir = StringParameter(nirvana_name='templates-root-dir', required=True)
        s3_bucket = StringParameter(nirvana_name='s3-bucket', required=True)
        s3_key = StringParameter(nirvana_name='s3-key', required=True)
        aws_access_key_id = StringParameter(nirvana_name='aws-access-key-id', required=True)
        aws_secret_access_key = SecretParameter(nirvana_name='aws-secret-access-key', required=True)

    class Inputs(JobProcessorOperation.Inputs):
        pass

    class Outputs(JobProcessorOperation.Outputs):
        templates = MRDirectoryOutput(nirvana_name='templates')
        report = TextOutput(nirvana_name='report')

    class Ports(JobProcessorOperation.Ports):
        pass

    def write_error_and_fail(self, message: str, exception: Optional[Exception]):
        Path(self.Outputs.report.get_path()).write_text(message)
        if exception:
            raise exception
        sys.exit(1)

    def exec(self):
        yt.config['proxy']['url'] = 'hahn'

        s3 = create_client(
            access_key_id=self.Parameters.aws_access_key_id,
            secret_access_key=self.Parameters.aws_secret_access_key
        )

        s3.download_file(
            Bucket=self.Parameters.s3_bucket,
            Key=self.Parameters.s3_key,
            Filename='archive.tar.gz'
        )

        if not tarfile.is_tarfile('archive.tar.gz'):
            self.write_error_and_fail('Загруженный файл не являет .tar.gz архивом', None)
        with tarfile.open('archive.tar.gz', 'r:gz') as tar:
            tar.extractall()

        if not Path('variables').exists():
            Path('variables').mkdir(parents=True)

        try:
            check_templates('templates.tsv', 'variables')
        except Exception as e:
            self.write_error_and_fail(str(e), e)

        templates = []
        for line in Path('templates.tsv').read_text().splitlines():
            templates.append({'template': line})

        variables = []
        for source in Path('variables').iterdir():
            for line in source.read_text().splitlines():
                variables.append({'source': source.name, 'variable': line})

        templates_root_dir = str(self.Parameters.templates_root_dir).rstrip('/')
        templates_dir = f'{templates_root_dir}/{self.Parameters.folder_id}/{self.Parameters.dataset_name}/{uuid4()}'
        templates_table = f'{templates_dir}/templates'
        variables_table = f'{templates_dir}/variables'

        assert not yt.exists(templates_dir)
        yt.mkdir(templates_dir, recursive=True)
        yt.write_table(templates_table, templates)
        yt.write_table(variables_table, variables)

        Path(self.Outputs.templates.get_path()).write_text(json.dumps({'cluster': 'hahn', 'path': templates_dir}))
        Path(self.Outputs.report.get_path()).write_text('Шаблоны успешно загружены')

    def run(self):
        self.exec()


class GenTexts(JobProcessorOperation):
    name = 'Generate texts'
    owner = 'cloud-ml'
    description = 'Generate texts from templates'

    class Parameters(JobProcessorOperation.Parameters):
        dataset_name = StringParameter(nirvana_name='dataset-name', required=True)
        folder_id = StringParameter(nirvana_name='folder-id', required=True)
        default_count = IntegerParameter(nirvana_name='default-count', required=True)
        output_table = StringParameter(nirvana_name='output-table', required=True)
        append_table = BooleanParameter(nirvana_name='append-table', required=False)
        fail_on_exists = BooleanParameter(nirvana_name='fail-on-exists', required=False)
        max_text_length = IntegerParameter(nirvana_name='max-text-length', required=True)

    class Inputs(JobProcessorOperation.Inputs):
        templates = MRDirectoryInput(nirvana_name='templates')
        resolution = JSONInput(nirvana_name='resolution')
        comments = JSONInput(nirvana_name='comments')

    class Outputs(JobProcessorOperation.Outputs):
        texts = MRTableOutput(nirvana_name='texts')
        report = TextOutput(nirvana_name='report')

    class Ports(JobProcessorOperation.Ports):
        pass

    def exec(self):
        yt.config['proxy']['url'] = 'hahn'

        resolution = json.loads(Path(self.Inputs.resolution[0].get_path()).read_text())['resolution']

        if resolution == 'won\'tFix':
            Path(self.Outputs.report.get_path()).write_text(
                'По результатам дополнительной проверки загруженные шаблоны были отклонены.\n'
                'Для уточнения деталей обратитесь в службу поддержки.'
            )
            sys.exit(0)
        assert resolution == 'fixed'

        templates_dir = json.loads(Path(self.Inputs.templates[0].get_path()).read_text())['path']
        templates_table = f'{templates_dir}/templates'
        variables_table = f'{templates_dir}/variables'

        texts_table = str(self.Parameters.output_table)
        if yt.exists(texts_table):
            if self.Parameters.fail_on_exists:
                raise ValueError(f'Table {texts_table} already exists')
        texts_table = yt.TablePath(texts_table, append=bool(self.Parameters.append_table))

        templates = []
        for row in yt.read_table(templates_table):
            templates.append(row['template'])

        variables = defaultdict(list)
        for row in yt.read_table(variables_table):
            variables[row['source']].append(row['variable'])

        comments = json.loads(Path(self.Inputs.comments[0].get_path()).read_text())

        count = self.Parameters.default_count
        for comment in comments:
            text = comment['text'].strip()
            if text.isdigit():
                count = int(text)

        rows = []
        while count > 0:
            template = random.choice(templates)
            text = generate_text(template, variables)
            if len(text) <= self.Parameters.max_text_length:
                count -= 1
                rows.append({'text': text, 'uuid': str(uuid4()), 'dataset': self.Parameters.dataset_name, 'folder': self.Parameters.folder_id})
        yt.write_table(texts_table, rows)

        Path(self.Outputs.texts.get_path()).write_text(json.dumps({'cluster': 'hahn', 'table': self.Parameters.output_table}))
        Path(self.Outputs.report.get_path()).write_text('Шаблоны успешно загружены')

    def run(self):
        self.exec()
