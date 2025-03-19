from typing import Dict, Optional

import torch
import torch.distributed as dist
from torch.nn.parallel import DistributedDataParallel as DDP
import torch.cuda.amp as amp

from acoustic_model.acoustic_model import AcousticModel
from acoustic_model.data import AcousticBatch
from acoustic_model.data.text_processor import TextProcessorBase
from acoustic_model.discriminator import MultiResolutionDiscriminator
from acoustic_model.loss import AcousticModelLoss
from core.lr_schedulers import create_scheduler
from core.optimizers import create_optimizer
from core.train_module import TrainModule
from core.utils import get_rank, get_random_segments, get_segments


class AcousticTrainModule(TrainModule):
    def __init__(
        self,
        model_config: dict,
        speakers: dict,
        optimizer_config: dict,
        text_processor: TextProcessorBase,
        use_amp: bool
    ):
        super().__init__()
        self.model_config = model_config
        self.speakers = speakers
        self.optimizer_config = optimizer_config
        self.use_amp = use_amp

        self.acoustic_model = AcousticModel(
            model_config=model_config,
            speakers=speakers,
            text_processor=text_processor
        )

        self.discriminator = self.segment_size = None
        if model_config.get("discriminator"):
            self.discriminator = MultiResolutionDiscriminator()
            self.segment_size = model_config["discriminator"]["segment_size"]

        self.acoustic_loss = AcousticModelLoss()

    def save_hyperparameters(self) -> Optional[Dict]:
        return {
            "model_config": self.model_config,
            "optimizer_config": self.optimizer_config,
            "speakers": self.speakers
        }

    def training_step(self, batch: AcousticBatch, batch_idx):
        if self.discriminator is not None:
            acoustic_optimizer, discriminator_optimizer = self.optimizers()
            acoustic_scheduler, discriminator_scheduler = self.lr_schedulers()
        else:
            acoustic_optimizer = self.optimizers()
            acoustic_scheduler = self.lr_schedulers()
            discriminator_optimizer = discriminator_scheduler = None

        with amp.autocast(self.use_amp):
            model_output = self.acoustic_model(batch)
            acoustic_loss, acoustic_loss_terms = self.acoustic_loss(model_output, batch)
        mel_hat = model_output[0]

        if self.discriminator is not None:
            y_segments, start_indices = get_random_segments(
                batch.mels.transpose(1, 2), batch.mel_lengths, self.segment_size
            )
            y_hat_segments = get_segments(
                mel_hat.transpose(1, 2), start_indices, self.segment_size
            )

            # discriminator loss
            with amp.autocast(self.use_amp):
                disc_loss = self.discriminator(y_segments, y_hat_segments.detach())["disc_loss"]
            discriminator_optimizer(disc_loss)

            # generator loss
            with amp.autocast(self.use_amp):
                disc_loss_dict = self.discriminator(y_segments, y_hat_segments, generator=True)
            gen_loss = disc_loss_dict["gen_loss"]
            fmap_loss = disc_loss_dict["fmap_loss"]
            fm_scale = acoustic_loss.item() / fmap_loss.item()
            loss = acoustic_loss + gen_loss + fm_scale * fmap_loss
        else:
            loss = acoustic_loss
            fmap_loss = gen_loss = disc_loss = torch.tensor(0.).to(loss)

        acoustic_optimizer(loss)

        if acoustic_scheduler is not None:
            acoustic_scheduler.step()
        if discriminator_scheduler is not None:
            discriminator_scheduler.step()

        metrics = self._get_acoustic_metrics(acoustic_loss_terms, "train")
        metrics["lr_acoustic"] = acoustic_optimizer.get_lr()
        if self.discriminator is not None:
            metrics.update({
                "train/fmap_loss": fmap_loss,
                "train/disc_loss": disc_loss,
                "train/gen_loss": gen_loss,
                "lr_discriminator": discriminator_optimizer.get_lr()
            })
        return metrics

    def validation_step(self, batch: AcousticBatch, batch_idx):
        with amp.autocast(self.use_amp):
            model_output = self.acoustic_model(batch)
            loss_terms = self.acoustic_loss(model_output, batch)[-1]
        return self._get_acoustic_metrics(loss_terms, "val")

    def _get_acoustic_metrics(self, loss_terms: Dict, prefix: str) -> Dict:
        return {
            f"{prefix}/mel_loss": loss_terms["mel_loss"],
            f"{prefix}/dur_loss": loss_terms["dur_loss"],
            f"{prefix}/pitch_loss": loss_terms["pitch_loss"],
            f"{prefix}/align_loss": loss_terms["align_loss"]
        }

    def configure_optimizers(self):
        acoustic_optimizer = create_optimizer(
            self.optimizer_config["acoustic_model"], self.acoustic_model.parameters()
        )
        acoustic_lr_scheduler = None
        scheduler_config = self.optimizer_config["acoustic_model"].get("scheduler", {})
        if scheduler_config:
            acoustic_lr_scheduler = create_scheduler(scheduler_config, acoustic_optimizer)

        if self.discriminator is not None:
            discriminator_optimizer = create_optimizer(
                self.optimizer_config["discriminator"], self.discriminator.parameters()
            )
            discriminator_lr_scheduler = None
            scheduler_config = self.optimizer_config["discriminator"].get("scheduler", {})
            if scheduler_config:
                discriminator_lr_scheduler = create_scheduler(scheduler_config, discriminator_optimizer)
            return [acoustic_optimizer, discriminator_optimizer], [acoustic_lr_scheduler, discriminator_lr_scheduler]

        return [acoustic_optimizer], [acoustic_lr_scheduler]

    def ddp(self):
        assert dist.is_initialized(), "distributed learning has not been initialized"
        rank = get_rank()
        self.acoustic_model = DDP(self.acoustic_model, device_ids=[rank], output_device=[rank])
        if self.discriminator is not None:
            self.discriminator = DDP(self.discriminator, device_ids=[rank], output_device=[rank])
        return self
