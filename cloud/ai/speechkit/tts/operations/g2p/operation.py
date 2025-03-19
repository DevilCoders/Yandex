import json
import os
import subprocess
import sys

from nope import JobProcessorOperation
from nope.endpoints import BinaryInput, ExecutableInput, MRTableInput, MRTableOutput
from nope.parameters import BooleanParameter, IntegerParameter, SecretParameter, StringParameter


class G2P(JobProcessorOperation):
    name = 'G2P operation'
    owner = 'cloud-ml'
    description = 'G2P operation'

    class Parameters(JobProcessorOperation.Parameters):
        output_table = StringParameter(nirvana_name='output-table', required=True)
        text_column = StringParameter(nirvana_name='text-column', default_value='text')
        result_column = StringParameter(nirvana_name='result-column', default_value='g2p_tokens')
        drop_sil = BooleanParameter(nirvana_name='drop-sil', default_value=False)
        connections = IntegerParameter(nirvana_name='connections', default_value=1)
        read_threads = IntegerParameter(nirvana_name='read-threads', default_value=1)
        print_period = IntegerParameter(nirvana_name='print-period', default_value=100)
        yt_token = SecretParameter(nirvana_name='yt-token', required=True)

    class Inputs(JobProcessorOperation.Inputs):
        input_table = MRTableInput(nirvana_name='input')
        cli = ExecutableInput(nirvana_name='cli')
        resources = BinaryInput(nirvana_name='resources')

    class Outputs(JobProcessorOperation.Outputs):
        output_table = MRTableOutput(nirvana_name='output')

    def exec(self):
        cli = self.Inputs.cli[0].get_path()

        subprocess.run(['tar', 'xvf', self.Inputs.resources[0].get_path()],
                       stdout=sys.stdout,
                       stderr=sys.stderr)

        input_table = json.load(open(self.Inputs.input_table[0].get_path()))
        input_table = input_table["table"]

        command = [
            cli,
            '--config', os.path.join(os.getcwd(), 'resources_config.json'),
            '--input-table', input_table,
            '--output-table', self.Parameters.output_table,
            '--drop-sil', str(self.Parameters.drop_sil).lower()
        ]
        if self.Parameters.result_column:
            command += ['--result-column', self.Parameters.result_column]
        if self.Parameters.text_column:
            command += ['--text-column', self.Parameters.text_column]
        if self.Parameters.connections:
            command += ['--connections', self.Parameters.connections]
        if self.Parameters.read_threads:
            command += ['--read-threads', self.Parameters.read_threads]
        if self.Parameters.print_period:
            command += ['--print-period', self.Parameters.print_period]

        subprocess.run(['ls', '-la'])
        command = [str(x) for x in command]
        sys.stdout.write(' '.join(command) + '\n')

        subprocess.run(command, stdout=sys.stdout, stderr=sys.stderr)

        json.dump({"cluster": "hahn", "table": self.Parameters.output_table},
                  open(self.Outputs.output_table.get_path(), "w"))

    def run(self):
        os.environ['YT_TOKEN'] = self.Parameters.yt_token
        self.exec()
