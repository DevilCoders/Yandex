from itertools import chain

import torch
import torch.cuda.amp as amp
import torch.distributed as dist
import torch.nn.functional as F
from torch.nn.parallel import DistributedDataParallel as DDP

from core.lr_schedulers import create_scheduler
from core.optimizers import create_optimizer
from core.train_module import TrainModule
from core.utils import get_rank
from vocoder.data import FeaturesExtractor, VocoderBatch
from vocoder.discriminators import MultiPeriodDiscriminator, MultiResolutionDiscriminator
from vocoder.loss import (
    calc_discriminator_loss,
    calc_generator_loss,
    MultiResolutionSTFTLoss
)
from vocoder.model import Vocoder


class VocoderTrainModule(TrainModule):
    def __init__(
        self,
        model_config: dict,
        features_config: dict,
        optimizer_config: dict,
        device: str,
        use_amp: bool
    ):
        super().__init__()
        self.model_config = model_config
        self.features_config = features_config
        self.optimizer_config = optimizer_config
        self.use_amp = use_amp

        self.features_extractor = FeaturesExtractor(
            n_fft=features_config["n_fft"],
            window_length=features_config["window_length"],
            hop_length=features_config["hop_length"],
            sample_rate=features_config["sample_rate"],
            num_mel_bins=features_config["num_mel_bins"],
            min_frequency=features_config["min_frequency"],
            max_frequency=features_config["max_frequency"],
            device=device
        )

        self.vocoder = Vocoder(model_config["vocoder"])
        self.mp_disc = MultiPeriodDiscriminator(model_config["multi_period_discriminator"]["periods"])
        resolutions = model_config["multi_resolution_discriminator"]["resolutions"]
        self.mr_disc = MultiResolutionDiscriminator(resolutions)
        self.stft_loss = MultiResolutionSTFTLoss(device=device, resolutions=resolutions)

    def save_hyperparameters(self):
        return {
            "model_config": self.model_config,
            "features_config": self.features_config,
            "optimizer_config": self.optimizer_config
        }

    def training_step(self, batch: VocoderBatch, batch_idx):
        generator_optimizer, discriminator_optimizer = self.optimizers()
        generator_scheduler, discriminator_scheduler = self.lr_schedulers()

        x, y = batch.mel, batch.audio

        # discriminator
        with amp.autocast(self.use_amp):
            y_g_hat = self.vocoder(x)
        y_g_hat = y_g_hat.float()  # cast to float due to fp16 issues with stft loss
        with amp.autocast(self.use_amp):
            y_df_hat_r, y_df_hat_g, _, _ = self.mp_disc(y, y_g_hat.detach())
            mrd_fake = self.mr_disc(y_g_hat.detach())
            mrd_real = self.mr_disc(y)
            loss_disc_f, losses_disc_f_r, losses_disc_f_g = calc_discriminator_loss(y_df_hat_r, y_df_hat_g)
            loss_disc_r, losses_disc_r_r, losses_disc_r_g = calc_discriminator_loss(mrd_real, mrd_fake)

        loss_disc_all = loss_disc_r + loss_disc_f
        discriminator_optimizer(loss_disc_all)

        # generator
        with amp.autocast(self.use_amp):
            y_df_hat_r, y_df_hat_g, fmap_f_r, fmap_f_g = self.mp_disc(y, y_g_hat)
            mrd_output = self.mr_disc(y_g_hat)

            loss_gen_f, losses_gen_f = calc_generator_loss(y_df_hat_g)
            loss_gen_r, losses_gen_r = calc_generator_loss(mrd_output)

            # Multi-Resolution STFT Loss
            sc_loss, mag_loss = self.stft_loss(y_g_hat.squeeze(1), y.squeeze(1))
            stft_loss = sc_loss + mag_loss

            loss_gen_all = loss_gen_r + loss_gen_f + 2.5 * stft_loss
        generator_optimizer(loss_gen_all)

        if generator_scheduler is not None:
            generator_scheduler.step()
        if discriminator_scheduler is not None:
            discriminator_scheduler.step()

        # mel loss for metrics
        with torch.no_grad(), amp.autocast(self.use_amp):
            y_g_hat_mel = self.features_extractor(y_g_hat.squeeze(1))
            loss_mel = F.l1_loss(y_g_hat_mel, x)

        return {
            "train/gen_loss": loss_gen_all,
            "train/disc_loss": loss_disc_all,
            "train/mel_loss": loss_mel,
            "lr_generator": generator_optimizer.get_lr(),
            "lr_discriminator": discriminator_optimizer.get_lr()
        }

    def validation_step(self, batch: VocoderBatch, batch_idx):
        x, y = batch.mel, batch.audio
        with amp.autocast(self.use_amp):
            y_g_hat = self.vocoder(x)
            y_g_hat_mel = self.features_extractor(y_g_hat.squeeze(1))
        return {"val/mel_loss": F.l1_loss(y_g_hat_mel, x)}

    def configure_optimizers(self):
        vocoder_optimizer = create_optimizer(
            self.optimizer_config["vocoder"], self.vocoder.parameters()
        )
        vocoder_lr_scheduler = None
        scheduler_config = self.optimizer_config["vocoder"].get("scheduler", {})
        if scheduler_config:
            vocoder_lr_scheduler = create_scheduler(scheduler_config, vocoder_optimizer)

        discriminator_params = chain(self.mp_disc.parameters(), self.mr_disc.parameters())
        discriminator_optimizer = create_optimizer(
            self.optimizer_config["discriminator"], discriminator_params
        )
        discriminator_lr_scheduler = None
        scheduler_config = self.optimizer_config["discriminator"].get("scheduler", {})
        if scheduler_config:
            discriminator_lr_scheduler = create_scheduler(scheduler_config, discriminator_optimizer)
        return [vocoder_optimizer, discriminator_optimizer], [vocoder_lr_scheduler, discriminator_lr_scheduler]

    def ddp(self):
        assert dist.is_initialized(), "distributed learning has not been initialized"
        rank = get_rank()
        self.vocoder = DDP(self.vocoder, device_ids=[rank], output_device=[rank])
        self.mp_disc = DDP(self.mp_disc, device_ids=[rank], output_device=[rank])
        self.mr_disc = DDP(self.mr_disc, device_ids=[rank], output_device=[rank])
        return self
