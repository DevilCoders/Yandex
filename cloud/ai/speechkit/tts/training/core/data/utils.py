from typing import Tuple

import torch


def workers_info() -> Tuple[int, int]:
    num_workers = 1 if not torch.distributed.is_initialized() else torch.distributed.get_world_size()
    my_rank = 0 if not torch.distributed.is_initialized() else torch.distributed.get_rank()
    data_load_process_info = torch.utils.data.get_worker_info()
    if data_load_process_info is not None:
        load_threads = data_load_process_info.num_workers
        thread_id = data_load_process_info.id
    else:
        load_threads = 1
        thread_id = 0
    part_id = num_workers * thread_id + my_rank
    return int(part_id), num_workers * load_threads
