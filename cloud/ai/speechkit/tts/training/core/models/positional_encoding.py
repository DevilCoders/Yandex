import math

import torch
from torch import Tensor
import torch.nn as nn


class PositionalEncoding(nn.Module):
    def __init__(self,
                 embedding_dim: int,
                 dropout: float,
                 max_length: int):
        super().__init__()
        embeddings = torch.zeros(max_length, embedding_dim)
        position = torch.arange(0, max_length, dtype=torch.float).unsqueeze(1)
        div_term = torch.exp(
            torch.arange(0, embedding_dim, 2).float() * (-math.log(10000.0) / embedding_dim)
        )
        embeddings[:, 0::2] = torch.sin(position * div_term)[:, :math.ceil(embedding_dim / 2)]
        embeddings[:, 1::2] = torch.cos(position * div_term)[:, :math.floor(embedding_dim / 2)]
        embeddings.unsqueeze_(0)
        self.dropout = nn.Dropout(p=dropout)
        self.register_buffer("_embeddings", embeddings)

    def forward(self, x: Tensor) -> Tensor:
        output = x + self._embeddings[:, :x.size(1), :]
        return self.dropout(output)
