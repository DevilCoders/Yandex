import torch
import torch.nn as nn
import torch.nn.functional as F

from acoustic_model.data import AcousticBatch
from core.utils import mask_from_lengths


class AttentionCTCLoss(torch.nn.Module):
    def __init__(self, blank_logprob=-1):
        super(AttentionCTCLoss, self).__init__()
        self.log_softmax = torch.nn.LogSoftmax(dim=3)
        self.blank_logprob = blank_logprob
        self.CTCLoss = nn.CTCLoss(zero_infinity=True)

    def forward(self, attn_logprob, in_lens, out_lens):
        key_lens = in_lens
        query_lens = out_lens
        attn_logprob_padded = F.pad(input=attn_logprob,
                                    pad=(1, 0, 0, 0, 0, 0, 0, 0),
                                    value=self.blank_logprob)
        cost_total = 0.0
        for bid in range(attn_logprob.shape[0]):
            target_seq = torch.arange(1, key_lens[bid] + 1).unsqueeze(0)
            curr_logprob = attn_logprob_padded[bid].permute(1, 0, 2)
            curr_logprob = curr_logprob[:query_lens[bid], :, :key_lens[bid] + 1]
            curr_logprob = self.log_softmax(curr_logprob[None])[0]
            ctc_cost = self.CTCLoss(
                curr_logprob, target_seq, input_lengths=query_lens[bid:bid + 1],
                target_lengths=key_lens[bid:bid + 1])
            cost_total += ctc_cost
        cost = cost_total / attn_logprob.shape[0]
        return cost


class AttentionBinarizationLoss(torch.nn.Module):
    def __init__(self):
        super(AttentionBinarizationLoss, self).__init__()

    def forward(self, hard_attention, soft_attention, eps=1e-12):
        log_sum = torch.log(torch.clamp(soft_attention[hard_attention == 1],
                                        min=eps)).sum()
        return -log_sum / hard_attention.sum()


class AcousticModelLoss(nn.Module):
    def __init__(self,
                 mel_loss: bool = True,
                 mel_scale: float = 1,
                 dur_scale: float = 1,
                 pitch_scale: float = 1,
                 align_scale: float = 1):
        super().__init__()
        self.attn_loss = AttentionCTCLoss()
        self.bin_loss = AttentionBinarizationLoss()
        self.mel_loss = mel_loss
        self.mel_scale = mel_scale
        self.dur_scale = dur_scale
        self.pitch_scale = pitch_scale
        self.align_scale = align_scale

    def forward(self, model_output, batch: AcousticBatch):
        mel_pred, log_dur_pred, pitch_pred, attn_logprob, attn_soft, attn_hard, dur_tgt, pitch_tgt = model_output

        if self.mel_loss:
            mel_loss = F.l1_loss(mel_pred, batch.mels, reduction="none")
            mel_mask = mask_from_lengths(batch.mel_lengths).unsqueeze(2).float().to(mel_pred)
            mel_loss = (mel_loss * mel_mask).mean()
        else:
            mel_loss = torch.tensor(0.).to(mel_pred)

        input_mask = mask_from_lengths(batch.input_lengths).float().to(mel_pred)

        if log_dur_pred is not None:
            dur_loss = F.l1_loss(log_dur_pred, (dur_tgt.float() + 1).log(), reduction="none")
            dur_loss = (dur_loss * input_mask).sum() / input_mask.sum()
        else:
            dur_loss = torch.tensor(0.).to(mel_loss)

        if pitch_pred is not None:
            pitch_loss = F.l1_loss(pitch_pred, pitch_tgt, reduction="none")
            pitch_loss = (pitch_loss * input_mask).sum() / input_mask.sum()
        else:
            pitch_loss = torch.tensor(0.).to(mel_loss)

        if attn_logprob is not None:
            attn_loss = self.attn_loss(attn_logprob, batch.input_lengths, batch.mel_lengths)
            bin_loss = self.bin_loss(attn_hard, attn_soft)
            align_loss = attn_loss + bin_loss
        else:
            align_loss = torch.tensor(0.).to(mel_loss)

        loss = self.mel_scale * mel_loss \
               + self.dur_scale * dur_loss \
               + self.pitch_scale * pitch_loss \
               + self.align_scale * align_loss

        loss_terms = {
            'mel_loss': mel_loss,
            'dur_loss': dur_loss,
            'pitch_loss': pitch_loss,
            'align_loss': align_loss
        }

        return loss, loss_terms


class DiscriminatorLoss(nn.Module):
    def __init__(self):
        super().__init__()

    def forward(self, mrd_real, mrd_fake, fmap_real, fmap_fake, generator: bool = False):
        if not generator:
            disc_loss, _, _ = self._discriminator_loss(mrd_real, mrd_fake)
            return {"disc_loss": disc_loss}
        fmap_loss = self._feature_loss(fmap_real, fmap_fake)
        gen_loss, _ = self._generator_loss(mrd_fake)
        return {"gen_loss": gen_loss, "fmap_loss": fmap_loss}

    def _feature_loss(self, fmap_r, fmap_g):
        loss = 0
        for dr, dg in zip(fmap_r, fmap_g):
            for rl, gl in zip(dr, dg):
                loss += torch.mean(torch.abs(rl - gl))
        return loss * 2

    def _discriminator_loss(self, disc_real_outputs, disc_generated_outputs):
        loss = 0
        r_losses = []
        g_losses = []
        for dr, dg in zip(disc_real_outputs, disc_generated_outputs):
            r_loss = torch.mean((1 - dr) ** 2)
            g_loss = torch.mean(dg ** 2)
            loss += (r_loss + g_loss)
            r_losses.append(r_loss.item())
            g_losses.append(g_loss.item())
        return loss, r_losses, g_losses

    def _generator_loss(self, disc_outputs):
        loss = 0
        gen_losses = []
        for dg in disc_outputs:
            l = torch.mean((1 - dg) ** 2)
            gen_losses.append(l)
            loss += l
        return loss, gen_losses
