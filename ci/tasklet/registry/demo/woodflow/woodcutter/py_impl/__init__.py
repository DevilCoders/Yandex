import time

from tasklet.services.ci import get_ci_env
from ci.tasklet.registry.demov2.woodflow.common.proto import woodflow_pb2
from ci.tasklet.registry.demo.woodflow.woodcutter.proto import woodcutter_tasklet
from ci.tasklet.common.proto import service_pb2 as ci


class WoodcutterImpl(woodcutter_tasklet.WoodcutterBase):

    def run(self):
        trees = self.input.trees or self.default_trees()
        progress_msg = ci.TaskletProgress()
        progress_msg.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress_msg.ci_env = get_ci_env(self.input.context)

        for i, tree in enumerate(trees):
            time.sleep(1)
            progress_msg.progress = float(i + 1) / len(trees)
            progress_msg.text = 'срублено деревьев {}'.format(i + 1)
            self.ctx.ci.UpdateProgress(progress_msg)

            timber = self.output.timbers.add()
            timber.name = 'бревно из дерева ' + tree.type

        progress_msg.status = ci.TaskletProgress.Status.SUCCESSFUL
        self.ctx.ci.UpdateProgress(progress_msg)

    def default_trees(self):
        return [
            self.make_tree('Бамбук обыкновенный'),
            self.make_tree('Ясень высокий'),
        ]

    def make_tree(self, name):
        tree = woodflow_pb2.Tree()
        tree.type = name
        return tree
