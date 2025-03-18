import time

from tasklet.services.ci import get_ci_env
from ci.tasklet.registry.demo.woodflow.furniture_factory.proto import furniture_factory_tasklet
from ci.tasklet.common.proto import service_pb2 as ci


class FurnitureFactoryImpl(furniture_factory_tasklet.FurnitureFactoryBase):
    WARDROBE_BOARDS_COUNT = 3

    def run(self):

        # собираем доски по типам
        boards = {}
        for board in self.input.boards:
            boards.setdefault(board.source.name, []).append(board)

        progress_msg = ci.TaskletProgress()
        progress_msg.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress_msg.ci_env = get_ci_env(self.input.context)
        total_groups = len(boards)
        progress = 0
        for board_type, same_boards in boards.items():
            for chunk in self.chunks(same_boards, FurnitureFactoryImpl.WARDROBE_BOARDS_COUNT):
                time.sleep(1)
                if len(chunk) < FurnitureFactoryImpl.WARDROBE_BOARDS_COUNT:
                    self.output.remain.extend(chunk)
                    continue

                furniture = self.output.furnitures.add()
                furniture.type = "Шкаф"
                furniture.description = self.describe_wardrobe(chunk)

            progress += 1
            progress_msg.progress = float(progress) / total_groups
            progress_msg.text = '{} использованы'.format(board_type)
            self.ctx.ci.UpdateProgress(progress_msg)

        progress_msg.status = ci.TaskletProgress.Status.SUCCESSFUL
        self.ctx.ci.UpdateProgress(progress_msg)

    def describe_wardrobe(self, boards):
        producers = {board.producer for board in boards}
        return "Шкаф из {} досок, полученных из материала '{}', произведенного {}".format(
            len(boards),
            boards[0].source.name,
            " и ".join(sorted(producers))
        )

    def chunks(self, list, count):
        for i in range(0, len(list), count):
            chunk = list[i: i + count]
            yield chunk
