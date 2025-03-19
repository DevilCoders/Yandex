import argparse
import os
from typing import Optional

import sys
from os.path import abspath, dirname

# enabling modules discovery from global entrypoint
sys.path.append(abspath(dirname(__file__) + "/../../"))

from acoustic_model.data.text_processor import get_text_processor_by_lang
from acoustic_model import AcousticTrainModule
from acoustic_model.acoustic_model import AcousticModel, InferenceDecoder, InferenceEncoder

import torch


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=str, required=True)
    parser.add_argument("--opset-version", type=int, default=10)
    parser.add_argument("--output-dir", type=str, default="onnx_models")
    parser.add_argument("--lang", type=str, default="ru")
    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    checkpoint = torch.load(args.checkpoint, map_location="cpu")
    text_processor = get_text_processor_by_lang(args.lang)
    model_config = checkpoint["hparams"]["model_config"]
    train_module = AcousticTrainModule(
        model_config=model_config,
        speakers=checkpoint["hparams"]["speakers"],
        optimizer_config=checkpoint["hparams"]["optimizer_config"],
        text_processor=text_processor,
        use_amp=False
    )
    train_module.load_state_dict(checkpoint["state_dict"])
    model = train_module.acoustic_model

    template_dim = None
    if model.is_adaptive:
        template_dim = model_config["templates"]["encoder"]["d_model"]

    print("Encoder export:")
    export_encoder(model, model_config["encoder"]["d_model"], template_dim, args.output_dir, args.opset_version)

    print("Decoder export:")
    export_decoder(model, model_config["decoder"]["d_model"], args.output_dir, args.opset_version)


def export_encoder(model: AcousticModel,
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
        if model.is_adaptive:
            dummy_template = torch.randn(batch_size, template_length, template_dim)

        encoder(dummy_input, dummy_template)

    input_names = ["features", "template"] if model.is_adaptive else ["features"]
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
    if model.is_adaptive:
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


def export_decoder(model: AcousticModel, features_dim: int, output_dir: str, opset_version: int):
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


if __name__ == "__main__":
    main()
