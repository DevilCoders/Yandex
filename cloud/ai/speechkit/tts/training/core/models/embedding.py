from typing import Optional

import torch.nn as nn
from torch import Tensor


class Embedding(nn.Embedding):
    def __init__(self,
                 num_embeddings: int,
                 embedding_dim: int,
                 padding_idx: Optional[int] = 0,
                 **kwargs):
        super().__init__(num_embeddings, embedding_dim, padding_idx=padding_idx, **kwargs)
        self.reset_parameters()

    def reset_parameters(self):
        nn.init.normal_(self.weight, mean=0, std=self.embedding_dim ** -0.5)
        nn.init.constant_(self.weight[self.padding_idx], 0)

    def forward(self, x: Tensor) -> Tensor:
        return super().forward(x)
