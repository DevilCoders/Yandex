import time

from tasklet.services.ci import get_ci_env
from ci.tasklet.registry.demo.woodflow.sawmill.proto import sawmill_tasklet
from ci.tasklet.common.proto import service_pb2 as ci


class SawmillImpl(sawmill_tasklet.SawmillBase):

    def run(self):
        if self.input.HasField('document'):
            title = self.input.document.title
            boards_per_timber = self.input.document.boards_per_timber
        else:
            title = 'Обычная лесопилка'
            boards_per_timber = 3

        total_boards = len(self.input.timbers) * boards_per_timber
        progress = 1

        for timber in self.input.timbers:
            progress_msg = ci.TaskletProgress()
            progress_msg.job_instance_id.CopyFrom(self.input.context.job_instance_id)
            progress_msg.ci_env = get_ci_env(self.input.context)

            for i in range(boards_per_timber):
                time.sleep(1)
                progress_msg.progress = float(progress) / total_boards
                progress_msg.text = 'произведено бревен {}'.format(progress)
                self.ctx.ci.UpdateProgress(progress_msg)

                board = self.output.boards.add()
                board.seq = i + 1
                board.producer = title
                board.source.CopyFrom(timber)

                progress += 1
