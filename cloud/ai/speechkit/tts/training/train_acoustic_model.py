import argparse
import json
import os

import torch

from acoustic_model.data import AcousticSampleBuilder, collate_wrapper
from acoustic_model.data.text_processor import get_text_processor_by_lang
from acoustic_model import AcousticTrainModule
from core.data import TtsDataModule
from core.loggers import ConsoleLogger, TensorBoardLogger
from core.callbacks import ModelCheckpoint, NirvanaCheckpoint
from core.trainer import Trainer
from core.utils import rank_zero_info, seed_everything


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--local_rank", type=int, default=None)

    parser.add_argument("--data-config", type=str, default="acoustic_model/configs/data.json")
    parser.add_argument("--model-config", type=str, default="acoustic_model/configs/model.json")
    parser.add_argument("--optimizer-config", type=str, default="acoustic_model/configs/optimizer.json")
    parser.add_argument("--speakers", type=str, default="acoustic_model/configs/speakers.json")
    # parser.add_argument("--text-processor-config", type=str, default="acoustic_model/configs/text_processor.json")
    parser.add_argument("--adaptive-mask-config", type=str, default=None)

    parser.add_argument("--checkpoint-dir", type=str, default="checkpoint")
    parser.add_argument("--logs-dir", type=str, default="tensorboard")
    parser.add_argument("--nirvana-checkpoint-path", type=str, default=None)
    parser.add_argument("--nirvana-logs-path", type=str, default=None)
    parser.add_argument("--previous-checkpoint-path", type=str, default=None)

    parser.add_argument("--lang", type=str, default='ru')

    parser.add_argument("--num-steps", type=int, default=100000)
    parser.add_argument("--log-interval", type=int, default=10)
    parser.add_argument("--val-interval", type=int, default=100)
    parser.add_argument("--checkpoint-interval", type=int, default=1000000000)
    parser.add_argument("--device", type=str, default="cuda")
    parser.add_argument("--use-amp", action="store_true")
    parser.add_argument("--num-workers", type=int, default=8)
    parser.add_argument("--seed", type=int, default=42)

    args = parser.parse_args()

    return args


def main(args):
    # os.environ['YT_TOKEN'] = 'xxx'
    if args.local_rank is not None:
        torch.cuda.set_device(args.local_rank)
        torch.distributed.init_process_group(backend="nccl", init_method="env://")

    for key, value in vars(args).items():
        rank_zero_info(f"{key}: {value}")

    seed_everything(args.seed)

    with open(args.model_config) as f:
        model_config = json.load(f)

    with open(args.speakers) as f:
        speakers = json.load(f)

    if args.adaptive_mask_config is not None:
        with open(args.adaptive_mask_config) as f:
            adaptive_mask_config = json.load(f)
    else:
        adaptive_mask_config = None

    if "templates" in model_config:
        assert adaptive_mask_config is not None, "mask config is None"
        variables_config = {
            "mel_dim": model_config["mel_dim"],
            "mask_config": adaptive_mask_config,
            "coalescence_window_size": model_config["templates"]["coalescence_window_size"],
            "coalescence_stride": model_config["templates"]["coalescence_stride"]
        }
    else:
        variables_config = None

    if isinstance(args.optimizer_config, str):
        with open(args.optimizer_config) as f:
            optimizer_config = json.load(f)
    else:
        assert isinstance(args.optimizer_config, dict)
        optimizer_config = args.optimizer_config

    with open(args.data_config) as f:
        data_config = json.load(f)

    text_processor = get_text_processor_by_lang(args.lang)
    sample_builder = AcousticSampleBuilder(
        text_processor=text_processor,
        variables_config=variables_config,
        speakers=speakers,
    )

    checkpoint_callback = ModelCheckpoint(
        checkpoint_dir=args.checkpoint_dir,
        checkpoint_interval=args.checkpoint_interval,
    )
    nirvana_checkpoint_callback = NirvanaCheckpoint(
        checkpoint_interval=args.checkpoint_interval,
        checkpoint_dir=args.checkpoint_dir,
        logs_dir=args.logs_dir,
        nirvana_checkpoint_path=args.nirvana_checkpoint_path,
        nirvana_logs_path=args.nirvana_logs_path
    )

    acoustic_model = AcousticTrainModule(
        model_config=model_config,
        speakers=speakers,
        optimizer_config=optimizer_config,
        text_processor=text_processor,
        use_amp=True
    )

    checkpoint_path = None
    if os.path.exists(os.path.join(args.checkpoint_dir, "checkpoint.ckpt")):
        checkpoint_path = os.path.join(args.checkpoint_dir, "checkpoint.ckpt")
        rank_zero_info(f"will continue from last checkpoint")
    elif args.previous_checkpoint_path is not None:
        nirvana_checkpoint_callback.restore_from_nirvana(args.checkpoint_dir, args.previous_checkpoint_path)
        torch.distributed.barrier()
        assert os.path.exists(os.path.join(args.checkpoint_dir, "checkpoint.ckpt"))
        acoustic_model.load_from_checkpoint(os.path.join(args.checkpoint_dir, "checkpoint.ckpt"))
        rank_zero_info(f"loaded previous model weights from: {args.previous_checkpoint_path}")

    data_module = TtsDataModule(
        data_config=data_config,
        sample_builder=sample_builder,
        collate_fn=collate_wrapper,
        num_workers=args.num_workers,
        pin_memory=False if args.device == "cpu" else True
    )

    trainer = Trainer(
        max_steps=args.num_steps,
        log_every_n_steps=args.log_interval,
        val_check_interval=args.val_interval,
        logger=[
            TensorBoardLogger(args.logs_dir),
            ConsoleLogger()
        ],
        callbacks=[
            checkpoint_callback,
            nirvana_checkpoint_callback
        ],
        gradient_clip_val=optimizer_config.get("clip_val"),
        device=args.device,
        use_amp=args.use_amp
    )
    trainer.fit(
        model=acoustic_model,
        data_module=data_module,
        ckpt_path=checkpoint_path
    )

    nirvana_checkpoint_callback.save()


if __name__ == "__main__":
    args = parse_args()
    main(args)
