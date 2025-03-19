from typing import List, Tuple

import torch
from torch import Tensor
import torch.nn as nn
import torch.nn.functional as F
from torch.nn.utils import weight_norm, spectral_norm

from acoustic_model.loss import DiscriminatorLoss


class Discriminator(nn.Module):
    def __init__(self, leaky_relu_slope: float = 0.2, use_spectral_norm: bool = False):
        super().__init__()
        self.leaky_relu_slope = leaky_relu_slope
        norm_layer = weight_norm if not use_spectral_norm else spectral_norm
        self.layers = nn.ModuleList([
            norm_layer(nn.Conv2d(1, 32, (3, 9), padding=(1, 4))),
            norm_layer(nn.Conv2d(32, 32, (3, 9), stride=(1, 2), padding=(1, 4))),
            norm_layer(nn.Conv2d(32, 32, (3, 9), stride=(1, 2), padding=(1, 4))),
            norm_layer(nn.Conv2d(32, 32, (3, 9), stride=(1, 2), padding=(1, 4))),
            norm_layer(nn.Conv2d(32, 32, (3, 3), padding=(1, 1))),
        ])
        self.final_conv = norm_layer(nn.Conv2d(32, 1, (3, 3), padding=(1, 1)))

    def forward(self, x: Tensor) -> Tuple[Tensor, List[Tensor]]:
        fmap = []
        x = x.unsqueeze(1)
        for layer in self.layers:
            x = layer(x)
            x = F.leaky_relu(x, self.leaky_relu_slope)
            fmap.append(x)
        x = self.final_conv(x)
        fmap.append(x)
        x = torch.flatten(x, 1, -1)
        return x, fmap


class MultiResolutionDiscriminator(nn.Module):
    def __init__(self, num_disc: int = 3):
        super().__init__()
        self.discriminators = nn.ModuleList(
            [Discriminator() for _ in range(num_disc)]
        )
        self.avg_pool = nn.ModuleList([nn.AvgPool1d(3, 1, padding=1) for _ in range(num_disc)])
        self.criterion = DiscriminatorLoss()

    def forward(self, y: Tensor, y_hat: Tensor, generator: bool = False):
        y_d_rs, y_d_gs = [], []
        fmap_rs, fmap_gs = [], []
        for i, disc in enumerate(self.discriminators):
            if i > 0:
                y = self.avg_pool[i - 1](y)
                y_hat = self.avg_pool[i - 1](y_hat)
            y_d_r, fmap_r = disc(y)
            y_d_g, fmap_g = disc(y_hat)
            y_d_rs.append(y_d_r)
            fmap_rs.append(fmap_r)
            y_d_gs.append(y_d_g)
            fmap_gs.append(fmap_g)
        return self.criterion(y_d_rs, y_d_gs, fmap_rs, fmap_gs, generator)
