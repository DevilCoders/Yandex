from itertools import chain
from typing import Dict, Optional

import torch
import torch.distributed as dist
import torch.nn.functional as F
from torch.nn.parallel import DistributedDataParallel as DDP
import torch.cuda.amp as amp

import acoustic_model.acoustic_model as acoustic_model
from acoustic_model.acoustic_model import AcousticModel
from acoustic_model.data.text_processor import TextProcessorBase
from acoustic_model.loss import AcousticModelLoss
from core.lr_schedulers import create_scheduler
from core.optimizers import create_optimizer
from core.train_module import TrainModule
from core.utils import get_rank, get_random_segments, get_segments
from e2e_model.data import End2EndBatch
from vocoder.discriminators import MultiPeriodDiscriminator, MultiResolutionDiscriminator
from vocoder.loss import (
    calc_discriminator_loss,
    calc_generator_loss,
    MultiResolutionSTFTLoss
)
from vocoder.data import FeaturesExtractor
from vocoder.model import Vocoder


class End2EndTrainModule(TrainModule):
    def __init__(
        self,
        model_config: dict,
        features_config: dict,
        speakers: dict,
        optimizer_config: dict,
        text_processor: TextProcessorBase,
        device: str,
        use_amp: bool
    ):
        super().__init__()
        self.model_config = model_config
        self._segment_size = self.model_config["segment_size"]
        self.features_config = features_config
        self._hop_length = features_config["hop_length"]
        self.speakers = speakers
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

        self.acoustic_model = AcousticModel(
            model_config=model_config["acoustic_model"],
            speakers=speakers,
            text_processor=text_processor
        )
        self.vocoder = Vocoder(model_config["vocoder"])
        self.mp_disc = MultiPeriodDiscriminator(model_config["multi_period_discriminator"]["periods"])
        resolutions = model_config["multi_resolution_discriminator"]["resolutions"]
        self.mr_disc = MultiResolutionDiscriminator(resolutions)

        self.acoustic_loss = AcousticModelLoss(mel_loss=False, align_scale=2.0)
        self.stft_loss = MultiResolutionSTFTLoss(device=device, resolutions=resolutions)

    def save_hyperparameters(self) -> Optional[Dict]:
        return {
            "model_config": self.model_config,
            "features_config": self.features_config,
            "optimizer_config": self.optimizer_config,
            "speakers": self.speakers
        }

    def training_step(self, batch: End2EndBatch, batch_idx):
        generator_optimizer, discriminator_optimizer = self.optimizers()
        generator_scheduler, discriminator_scheduler = self.lr_schedulers()

        with amp.autocast(self.use_amp):
            acoustic_output = self.acoustic_model(batch)
        acoustic_features = acoustic_output[0].transpose(1, 2)
        acoustic_features, start_idxs = get_random_segments(
            acoustic_features,
            batch.mel_lengths,
            self._segment_size,
        )
        audio = get_segments(
            batch.audio.unsqueeze(1),
            start_idxs * self._hop_length,
            self._segment_size * self._hop_length,
        )
        with amp.autocast(self.use_amp):
            audio_hat = self.vocoder(acoustic_features)

        # discriminator
        audio_hat = audio_hat.float()  # cast to float due to fp16 issues with stft loss
        with amp.autocast(self.use_amp):
            y_df_hat_r, y_df_hat_g, _, _ = self.mp_disc(audio, audio_hat.detach())
            mrd_fake = self.mr_disc(audio_hat.detach())
            mrd_real = self.mr_disc(audio)
            loss_disc_f, losses_disc_f_r, losses_disc_f_g = calc_discriminator_loss(y_df_hat_r, y_df_hat_g)
            loss_disc_r, losses_disc_r_r, losses_disc_r_g = calc_discriminator_loss(mrd_real, mrd_fake)
        loss_disc_all = loss_disc_r + loss_disc_f
        discriminator_optimizer(loss_disc_all)

        # generator
        with amp.autocast(self.use_amp):
            y_df_hat_r, y_df_hat_g, fmap_f_r, fmap_f_g = self.mp_disc(audio, audio_hat)
            mrd_output = self.mr_disc(audio_hat)

            loss_gen_f, losses_gen_f = calc_generator_loss(y_df_hat_g)
            loss_gen_r, losses_gen_r = calc_generator_loss(mrd_output)

            # Multi-Resolution STFT Loss
            sc_loss, mag_loss = self.stft_loss(audio_hat.squeeze(1), audio.squeeze(1))
            stft_loss = sc_loss + mag_loss

            acoustic_loss, acoustic_loss_terms = self.acoustic_loss(acoustic_output, batch)

            loss_gen_all = loss_gen_r + loss_gen_f + 2.5 * stft_loss + acoustic_loss
        generator_optimizer(loss_gen_all)

        if generator_scheduler is not None:
            generator_scheduler.step()
        if discriminator_scheduler is not None:
            discriminator_scheduler.step()

        # mel loss for metrics
        mel = get_segments(batch.mels.transpose(1, 2), start_idxs, self._segment_size)
        with torch.no_grad(), amp.autocast(self.use_amp):
            y_g_hat_mel = self.features_extractor(audio_hat.squeeze(1))
            loss_mel = F.l1_loss(y_g_hat_mel, mel)

        return {
            "train/gen_loss": loss_gen_all,
            "train/disc_loss": loss_disc_all,
            "train/mel_loss": loss_mel,
            **self._get_acoustic_metrics(acoustic_loss_terms, "train"),
            "lr_generator": generator_optimizer.get_lr(),
            "lr_discriminator": discriminator_optimizer.get_lr()
        }

    def infer(self, batch: End2EndBatch):
        with amp.autocast(self.use_amp), torch.no_grad():
            acoustic_features = self.acoustic_model.infer(batch)
            audio_hat = self.vocoder(acoustic_features.transpose(1, 2))
        return audio_hat

    def validation_step(self, batch: End2EndBatch, batch_idx):
        with amp.autocast(self.use_amp):
            acoustic_output = self.acoustic_model(batch)
            acoustic_loss_terms = self.acoustic_loss(acoustic_output, batch)[1]
        acoustic_features = acoustic_output[0]
        acoustic_features, start_idxs = get_random_segments(
            acoustic_features.transpose(1, 2),
            batch.mel_lengths,
            self._segment_size,
        )
        mel = get_segments(batch.mels.transpose(1, 2), start_idxs, self._segment_size)
        with amp.autocast(self.use_amp):
            audio_hat = self.vocoder(acoustic_features)
            y_g_hat_mel = self.features_extractor(audio_hat.squeeze(1))
        return {
            "val/mel_loss": F.l1_loss(y_g_hat_mel, mel),
            **self._get_acoustic_metrics(acoustic_loss_terms, "val"),
        }

    def _get_acoustic_metrics(self, loss_terms: Dict, prefix: str) -> Dict:
        return {
            f"{prefix}/dur_loss": loss_terms["dur_loss"],
            f"{prefix}/pitch_loss": loss_terms["pitch_loss"],
            f"{prefix}/align_loss": loss_terms["align_loss"]
        }

    def configure_optimizers(self):
        generator_optimizer = create_optimizer(
            self.optimizer_config["generator"], chain(self.acoustic_model.parameters(), self.vocoder.parameters())
        )
        generator_lr_scheduler = None
        scheduler_config = self.optimizer_config["generator"].get("scheduler", {})
        if scheduler_config:
            generator_lr_scheduler = create_scheduler(scheduler_config, generator_optimizer)

        discriminator_params = chain(self.mp_disc.parameters(), self.mr_disc.parameters())
        discriminator_optimizer = create_optimizer(self.optimizer_config["discriminator"], discriminator_params)
        discriminator_lr_scheduler = None
        scheduler_config = self.optimizer_config["discriminator"].get("scheduler", {})
        if scheduler_config:
            discriminator_lr_scheduler = create_scheduler(scheduler_config, discriminator_optimizer)
        return [generator_optimizer, discriminator_optimizer], [generator_lr_scheduler, discriminator_lr_scheduler]

    def ddp(self):
        assert dist.is_initialized(), "distributed learning has not been initialized"
        rank = get_rank()
        self.acoustic_model = DDP(self.acoustic_model, device_ids=[rank], output_device=[rank])
        self.vocoder = DDP(self.vocoder, device_ids=[rank], output_device=[rank])
        self.mp_disc = DDP(self.mp_disc, device_ids=[rank], output_device=[rank])
        self.mr_disc = DDP(self.mr_disc, device_ids=[rank], output_device=[rank])
        return self


class InferenceEncoder(torch.nn.Module):
    def __init__(self, model: End2EndTrainModule):
        super().__init__()
        self.encoder = acoustic_model.InferenceEncoder(model.acoustic_model)

    def forward(self, x, template=None):
        return self.encoder(x, template)


class InferenceDecoder(torch.nn.Module):
    def __init__(self, model: End2EndTrainModule):
        super().__init__()
        self.decoder = acoustic_model.InferenceDecoder(model.acoustic_model)

    def forward(self, x, pitch):
        return self.decoder(x, pitch)


class InferenceVocoder(torch.nn.Module):
    def __init__(self, model: End2EndTrainModule):
        super().__init__()
        self.vocoder = model.vocoder

    def forward(self, x):
        return self.vocoder(x.transpose(1, 2))
