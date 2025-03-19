import copy
from typing import Optional

import torch
from torch import Tensor
import torch.nn as nn
from torch.nn import (
    Dropout,
    LayerNorm,
    Linear,
    Module,
    ModuleList,
    MultiheadAttention
)


class LinearFeedforward(Module):
    def __init__(self,
                 d_model: int,
                 dim_feedforward: int,
                 activation: str,
                 dropout: float,
                 d_output: Optional[int] = None):
        super().__init__()
        d_output = d_model if d_output is None else d_output
        self.model = nn.Sequential(
            Linear(d_model, dim_feedforward),
            _get_activation_fn(activation),
            nn.Dropout(dropout),
            Linear(dim_feedforward, d_output)
        )

    def forward(self, x: Tensor) -> Tensor:
        return self.model(x)


class ConvFeedforward(nn.Module):
    def __init__(self,
                 d_model: int,
                 dim_feedforward: int,
                 kernel_size: int,
                 dropout: float,
                 d_output: Optional[int] = None):
        super().__init__()
        assert kernel_size % 2 == 1, "kernel size must be odd"
        d_output = d_model if d_output is None else d_output
        self.filter_conv = nn.Conv1d(
            in_channels=d_model,
            out_channels=dim_feedforward,
            kernel_size=kernel_size,
            padding=(kernel_size - 1) // 2
        )
        self.gate_conv = nn.Conv1d(
            in_channels=d_model,
            out_channels=dim_feedforward,
            kernel_size=kernel_size,
            padding=(kernel_size - 1) // 2
        )
        self.final_conv = nn.Conv1d(
            in_channels=dim_feedforward,
            out_channels=d_output,
            kernel_size=kernel_size,
            padding=(kernel_size - 1) // 2
        )
        # self.dropout = nn.Dropout(dropout)  // worse convergence according to FastPitch

    def forward(self, x: Tensor) -> Tensor:
        x = x.transpose(0, 1).transpose(1, 2)
        x_filter = torch.tanh(self.filter_conv(x))
        x_gate = torch.sigmoid(self.gate_conv(x))
        x = x_filter * x_gate
        # x = self.dropout(x)  // worse convergence according to FastPitch
        x = self.final_conv(x)
        x = x.transpose(1, 2).transpose(0, 1)
        return x


def _get_feedforward_model(model_type: str, d_model: int, dropout: float, config: dict, **kwargs):
    assert model_type in {"linear", "conv"}, f"unknown feedforward model: {model_type}"
    if model_type == "linear":
        model = LinearFeedforward(d_model=d_model, dropout=dropout, **config, **kwargs)
    else:
        model = ConvFeedforward(d_model=d_model, dropout=dropout, **config, **kwargs)
    return model


class TransformerEncoder(Module):
    def __init__(self,
                 num_layers: int,
                 d_model: int,
                 num_heads: int,
                 feedforward_config: dict,
                 dropout: float,
                 normalize_before: bool = True):
        super().__init__()
        feedforward_config = copy.deepcopy(feedforward_config)
        encoder_layer = TransformerEncoderLayer(d_model=d_model,
                                                num_heads=num_heads,
                                                feedforward_config=feedforward_config,
                                                dropout=dropout,
                                                normalize_before=normalize_before)
        self.d_model = d_model
        self.layers = _get_clones(encoder_layer, num_layers)

    def dim(self):
        return self.d_model

    def forward(self,
                x: Tensor,
                square_mask: Optional[Tensor] = None,
                padding_mask: Optional[Tensor] = None) -> Tensor:
        x = x.transpose(0, 1)
        for layer in self.layers:
            x = layer(x, src_mask=square_mask, src_key_padding_mask=padding_mask)
        x = x.transpose(0, 1)
        return x


class TransformerEncoderLayer(Module):
    def __init__(self,
                 d_model: int,
                 num_heads: int,
                 feedforward_config: dict,
                 dropout: float,
                 normalize_before: bool):
        super().__init__()
        self.self_attn = MultiheadAttention(d_model, num_heads, dropout=dropout)

        feedforward_type = feedforward_config.pop("type")
        self.feedforward = _get_feedforward_model(feedforward_type, d_model, dropout, feedforward_config)

        self.norm1 = LayerNorm(d_model)
        self.norm2 = LayerNorm(d_model)
        self.dropout1 = Dropout(dropout)
        self.dropout2 = Dropout(dropout)

        self.normalize_before = normalize_before

    def forward(self,
                x: Tensor,
                src_mask: Optional[Tensor] = None,
                src_key_padding_mask: Optional[Tensor] = None) -> Tensor:
        residual = x
        if self.normalize_before:
            x = self.norm1(x)
        x = self.self_attn(x, x, x, attn_mask=src_mask,
                           key_padding_mask=src_key_padding_mask)[0]
        x = residual + self.dropout1(x)
        if not self.normalize_before:
            x = self.norm1(x)

        residual = x
        if self.normalize_before:
            x = self.norm2(x)
        x = self.feedforward(x)
        x = self.dropout2(x)
        x = residual + x
        if not self.normalize_before:
            x = self.norm2(x)

        return x


class TransformerDecoder(Module):
    def __init__(self,
                 num_layers: int,
                 d_model: int,
                 key_value_dim: Optional[int],
                 num_heads: int,
                 feedforward_config: dict,
                 dropout: float,
                 normalize_before: bool = True):
        super().__init__()
        feedforward_config = copy.deepcopy(feedforward_config)
        decoder_layer = TransformerDecoderLayer(d_model=d_model,
                                                key_value_dim=key_value_dim,
                                                num_heads=num_heads,
                                                feedforward_config=feedforward_config,
                                                dropout=dropout,
                                                normalize_before=normalize_before)
        self.d_model = d_model
        self.layers = _get_clones(decoder_layer, num_layers)

    def dim(self):
        return self.d_model

    def forward(self,
                x: Tensor,
                memory: Tensor,
                input_padding_mask: Optional[Tensor] = None,
                memory_padding_mask: Optional[Tensor] = None) -> Tensor:
        x = x.transpose(0, 1)
        memory = memory.transpose(0, 1)
        for layer in self.layers:
            x = layer(x, memory,
                      tgt_key_padding_mask=input_padding_mask,
                      memory_key_padding_mask=memory_padding_mask)
        x = x.transpose(0, 1)
        return x


class TransformerDecoderLayer(Module):
    def __init__(self,
                 d_model: int,
                 key_value_dim: int,
                 num_heads: int,
                 feedforward_config: dict,
                 dropout: float,
                 normalize_before: bool):
        super().__init__()
        self.self_attn = MultiheadAttention(d_model, num_heads, dropout=dropout)
        self.multihead_attn = MultiheadAttention(
            d_model, num_heads, kdim=key_value_dim, vdim=key_value_dim, dropout=dropout
        )

        feedforward_type = feedforward_config.pop("type")
        self.feedforward = _get_feedforward_model(feedforward_type, d_model, dropout, feedforward_config)

        self.norm1 = LayerNorm(d_model)
        self.norm2 = LayerNorm(d_model)
        self.norm3 = LayerNorm(d_model)
        self.dropout1 = Dropout(dropout)
        self.dropout2 = Dropout(dropout)
        self.dropout3 = Dropout(dropout)

        self.normalize_before = normalize_before

    def forward(self,
                x: Tensor,
                memory: Tensor,
                tgt_mask: Optional[Tensor] = None,
                memory_mask: Optional[Tensor] = None,
                tgt_key_padding_mask: Optional[Tensor] = None,
                memory_key_padding_mask: Optional[Tensor] = None) -> Tensor:
        residual = x
        if self.normalize_before:
            x = self.norm1(x)
        x = self.self_attn(x, x, x, attn_mask=tgt_mask,
                           key_padding_mask=tgt_key_padding_mask)[0]
        x = residual + self.dropout1(x)
        if not self.normalize_before:
            x = self.norm1(x)

        residual = x
        if self.normalize_before:
            x = self.norm2(x)
        x = self.multihead_attn(x, memory, memory, attn_mask=memory_mask,
                                key_padding_mask=memory_key_padding_mask)[0]
        x = residual + self.dropout2(x)
        if not self.normalize_before:
            x = self.norm2(x)

        residual = x
        if self.normalize_before:
            x = self.norm3(x)
        x = self.feedforward(x)
        x = self.dropout3(x)
        x = residual + x
        if not self.normalize_before:
            x = self.norm3(x)

        return x


def _get_clones(module, n):
    return ModuleList([copy.deepcopy(module) for _ in range(n)])


def _get_activation_fn(activation):
    if activation == "relu":
        return nn.ReLU()
    elif activation == "gelu":
        return nn.GELU()

    raise RuntimeError(f"activation should be relu/gelu, not {activation}")
