import sys
from os.path import abspath, dirname

# enabling modules discovery from global entrypoint
sys.path.append(abspath(dirname(__file__) + "/../../"))

import argparse
import os
from typing import Optional

from acoustic_model.data.text_processor import TextProcessor
from e2e_model import (
    End2EndTrainModule,
    InferenceDecoder,
    InferenceEncoder,
    InferenceVocoder
)

import torch


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=str, required=True)
    parser.add_argument("--opset-version", type=int, default=10)
    parser.add_argument("--output-dir", type=str, default="onnx_models")
    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    checkpoint = torch.load(args.checkpoint, map_location="cpu")
    text_processor = TextProcessor.from_file("acoustic_model/configs/text_processor.json")
    model_config = checkpoint["hparams"]["model_config"]
    model = End2EndTrainModule(
        model_config=checkpoint["hparams"]["model_config"],
        features_config=checkpoint["hparams"]["features_config"],
        speakers=checkpoint["hparams"]["speakers"],
        optimizer_config=None,
        text_processor=text_processor,
        device="cuda",
        use_amp=True
    )
    model.load_state_dict(checkpoint["state_dict"])
    model.vocoder.remove_weight_norm()
    model = model.eval()

    template_dim = None
    if model.acoustic_model.is_adaptive:
        template_dim = model_config["acoustic_model"]["templates"]["encoder"]["d_model"]

    print("Encoder export:")
    export_encoder(
        model, model_config["acoustic_model"]["encoder"]["d_model"], template_dim, args.output_dir, args.opset_version
    )

    print("Decoder export:")
    export_decoder(
        model, model_config["acoustic_model"]["decoder"]["d_model"], args.output_dir, args.opset_version
    )

    print("Vocoder export:")
    export_vocoder(
        model, model_config["vocoder"]["num_mel_bins"], args.output_dir, args.opset_version
    )


def export_encoder(model: End2EndTrainModule,
                   features_dim: int,
                   template_dim: Optional[int],
                   output_dir: str,
                   opset_version: int):
    encoder = InferenceEncoder(model)
    encoder = encoder.eval()

    with torch.no_grad():
        batch_size = 1
        sequence_length = 512
        template_length = 512

        dummy_input = torch.randn(batch_size, sequence_length, features_dim)
        dummy_template = None
        if model.acoustic_model.is_adaptive:
            dummy_template = torch.randn(batch_size, template_length, template_dim)

        encoder(dummy_input, dummy_template)

    input_names = ["features", "template"] if model.acoustic_model.is_adaptive else ["features"]
    output_names = ["output_features", "durations", "pitch"]

    dynamic_axes = {
        "features": {
            0: 'batch_size',
            1: "num_frames"
        },
        "output_features": {
            0: "batch_size",
            1: "num_frames"
        },
        "durations": {
            0: "batch_size",
            1: "sequence_length"
        },
        "pitch": {
            0: "batch_size",
            1: "sequence_length"
        }
    }
    if model.acoustic_model.is_adaptive:
        dynamic_axes["template"] = {
            0: "batch_size",
            1: "num_frames"
        }

    torch.onnx.export(encoder,
                      (dummy_input, dummy_template),
                      os.path.join(output_dir, "encoder.onnx"),
                      opset_version=opset_version,
                      verbose=True,
                      export_params=True,
                      input_names=input_names,
                      output_names=output_names,
                      dynamic_axes=dynamic_axes)


def export_decoder(model: End2EndTrainModule, features_dim: int, output_dir: str, opset_version: int):
    decoder = InferenceDecoder(model)
    decoder = decoder.eval()

    with torch.no_grad():
        batch_size = 1
        sequence_length = 2048

        dummy_features = torch.randn(batch_size, sequence_length, features_dim)
        dummy_pitch = torch.randn(batch_size, sequence_length)
        decoder(dummy_features, dummy_pitch)

    input_names = ["features", "pitch"]
    output_names = ["spectrogram"]

    dynamic_axes = {
        "features": {
            0: "batch_size",
            1: "num_frames"
        },
        "pitch": {
            0: "batch_size",
            1: "sequence_length"
        },
        "spectrogram": {
            0: "batch_size",
            1: "num_frames"
        }
    }

    torch.onnx.export(decoder,
                      (dummy_features, dummy_pitch),
                      os.path.join(output_dir, "decoder.onnx"),
                      opset_version=opset_version,
                      verbose=True,
                      export_params=True,
                      input_names=input_names,
                      output_names=output_names,
                      dynamic_axes=dynamic_axes)


def export_vocoder(model: End2EndTrainModule, features_dim: int, output_dir: str, opset_version: int):
    vocoder = InferenceVocoder(model)
    vocoder = vocoder.eval()

    with torch.no_grad():
        batch_size = 1
        num_frames = 2048
        dummy_features = torch.randn(batch_size, num_frames, features_dim).float()
        out = vocoder(dummy_features)

    input_names = ["features"]
    output_names = ["audio"]
    dynamic_axes = {
        "features": {
            0: "batch_size",
            1: "num_frames"
        },
        "audio": {
            0: "batch_size",
            1: "audio_length"
        }
    }

    torch.onnx.export(vocoder,
                      dummy_features,
                      os.path.join(output_dir, "vocoder.onnx"),
                      opset_version=opset_version,
                      verbose=True,
                      export_params=True,
                      input_names=input_names,
                      output_names=output_names,
                      dynamic_axes=dynamic_axes)


if __name__ == "__main__":
    main()
