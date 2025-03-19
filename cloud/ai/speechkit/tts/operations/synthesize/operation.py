from email.policy import default
import json
import os
import subprocess
import sys

from nope import JobProcessorOperation
from nope.endpoints import *
from nope.parameters import *


class Synthesize(JobProcessorOperation, ):
    name = 'Synthesize'
    owner = 'cloud-ml'
    description = 'Synthesize'

    class Parameters(JobProcessorOperation.Parameters):
        host = StringParameter(nirvana_name='host', required=True)
        port = IntegerParameter(nirvana_name='port', required=True)
        enable_ssl = BooleanParameter(nirvana_name='enable-ssl')

        model = StringParameter(nirvana_name='model')
        unsafe_mode = BooleanParameter(nirvana_name='unsafe-mode')
        speaker = StringParameter(nirvana_name='speaker')
        audio_format = EnumParameter(enum_values=['pcm', 'wav', 'ogg_opus'], nirvana_name='audio-format')
        sample_rate = IntegerParameter(nirvana_name='sample-rate')

        speaker_column = StringParameter(nirvana_name='speaker-column')
        text_column = StringParameter(nirvana_name='text-column')
        result_column = StringParameter(nirvana_name='result-column')
        output_table = StringParameter(nirvana_name='output-table', required=True)

        connections = IntegerParameter(nirvana_name='connections')
        read_threads_number = IntegerParameter(nirvana_name='read-threads-number')

        yt_token = SecretParameter(nirvana_name='yt-token', required=True)
        api_token = SecretParameter(nirvana_name='api-token', required=True)

    class Inputs(JobProcessorOperation.Inputs):
        input_table = MRTableInput(nirvana_name='input')
        cli = ExecutableInput(nirvana_name='cli')

    class Outputs(JobProcessorOperation.Outputs):
        output_table = MRTableOutput(nirvana_name='output')

    def run(self):
        os.environ['YT_TOKEN'] = self.Parameters.yt_token

        cli = self.Inputs.cli[0].get_path()

        input_table = json.load(open(self.Inputs.input_table[0].get_path()))
        input_table = f'{input_table["cluster"]}/{input_table["table"]}'

        command = [
            cli, 'synthesize',
            '--input-table', input_table,
            '--output-table', f'hahn/{self.Parameters.output_table}',
            '--endpoint', f'{self.Parameters.host}:{self.Parameters.port}',
            '-H', f'authorization: api-Key {self.Parameters.api_token}'
        ]
        if self.Parameters.enable_ssl:
            command += ['--ssl']
        if self.Parameters.model:
            command += ['--model', self.Parameters.model]
        if self.Parameters.unsafe_mode:
            command += ['--unsafe']
        if self.Parameters.speaker:
            command += ['--speaker', self.Parameters.speaker]
        if self.Parameters.audio_format:
            command += ['--audio-format', self.Parameters.audio_format]
        if self.Parameters.sample_rate:
            command += ['--sample-rate', f'{self.Parameters.sample_rate}']
        if self.Parameters.text_column:
            command += ['--text-column', self.Parameters.text_column]
        if self.Parameters.speaker_column:
            command += ['--speaker-column', self.Parameters.speaker_column]
        if self.Parameters.result_column:
            command += ['--result-column', self.Parameters.result_column]
        if self.Parameters.connections:
            command += ['--connections', f'{self.Parameters.connections}']
        if self.Parameters.read_threads_number:
            command += ['--read-threads', f'{self.Parameters.read_threads_number}']

        sys.stdout.write(' '.join(command) + '\n')
        subprocess.run(command, stdout=sys.stdout, stderr=sys.stderr)

        json.dump({'cluster': 'hahn', 'table': self.Parameters.output_table},
                  open(self.Outputs.output_table.get_path(), 'w'))
