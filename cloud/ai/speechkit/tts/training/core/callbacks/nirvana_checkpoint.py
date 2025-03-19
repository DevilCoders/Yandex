import os
import sys
import subprocess as sp

from core.callbacks.base import Callback
from core.utils import get_rank, rank_zero_info, rank_zero_only


class NirvanaCheckpoint(Callback):
    def __init__(
        self,
        checkpoint_interval: int,
        checkpoint_dir: str,
        logs_dir: str,
        nirvana_checkpoint_path: str,
        nirvana_logs_path: str
    ):
        super().__init__()
        self.checkpoint_interval = checkpoint_interval
        self.checkpoint_dir = checkpoint_dir
        self.logs_dir = logs_dir
        self.nirvana_checkpoint_path = nirvana_checkpoint_path
        self.nirvana_logs_path = nirvana_logs_path

        if get_rank() == 0:
            os.makedirs(checkpoint_dir, exist_ok=True)
            os.makedirs(logs_dir, exist_ok=True)

        if nirvana_checkpoint_path and os.path.isfile(nirvana_checkpoint_path):
            rank_zero_info(f"restoring checkpoint directory from {nirvana_checkpoint_path}")
            self.restore_from_nirvana(checkpoint_dir, nirvana_checkpoint_path)

        if nirvana_logs_path and os.path.isfile(nirvana_logs_path):
            rank_zero_info(f"restoring log directory from {nirvana_logs_path}")
            self.restore_from_nirvana(logs_dir, nirvana_logs_path)

    @rank_zero_only
    def save(self):
        if self.nirvana_checkpoint_path is None:
            assert self.nirvana_logs_path is None
            return
        _archive_and_move(self.checkpoint_dir, self.nirvana_checkpoint_path)
        _archive_and_move(self.logs_dir, self.nirvana_logs_path)
        rank_zero_info("saved checkpoint and logs to nirvana output")

    @rank_zero_only
    def on_train_batch_end(
        self,
        trainer,
        module,
        batch,
        batch_idx: int
    ):
        if batch_idx < self.checkpoint_interval:
            return
        # save in next step after checkpoint
        if (batch_idx + 1) % self.checkpoint_interval != 1:
            return
        self.save()

    @rank_zero_only
    def restore_from_nirvana(self, dir_name: str, nirvana_path: str):
        sys.stderr.write(f"#{get_rank()}: restoring from nirvana\n")
        sp.check_call(f"tar xf {nirvana_path} --strip 1 -C {dir_name}", shell=True)


def _archive_and_move(source, target):
    parent_dir = os.path.dirname(os.path.abspath(source))
    sp.check_call(f"tar cf archive.tar -C {parent_dir} {os.path.basename(source)}", shell=True)
    sp.check_call(f"mv archive.tar {target}", shell=True)
