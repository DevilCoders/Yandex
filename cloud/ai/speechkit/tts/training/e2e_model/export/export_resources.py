import sys
from os.path import abspath, dirname

# enabling modules discovery from global entrypoint
sys.path.append(abspath(dirname(__file__) + "/../../"))

import argparse
import os

from acoustic_model.data.text_processor import TextProcessor
from e2e_model import End2EndTrainModule

import numpy as np
import torch


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=str, required=True)
    parser.add_argument("--output-dir", type=str, default="onnx_models")
    args = parser.parse_args()

    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    checkpoint = torch.load(args.checkpoint, map_location="cpu")
    text_processor = TextProcessor.from_file("acoustic_model/configs/text_processor.json")
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

    token_embeddings = model.acoustic_model.embedding.weight.data.numpy()
    speaker_embeddings = model.acoustic_model.speaker_embedding.weight.data.numpy()
    role_embeddings = model.acoustic_model.role_embedding.weight.data.numpy()
    positional_embeddings = model.acoustic_model.positional_encoding._embeddings.squeeze(0).data.numpy()
    accent_embedding = model.acoustic_model.accent_embedding.weight.data[1].numpy()
    embeddings = {
        "token_embeddings": token_embeddings,
        "speaker_embeddings": speaker_embeddings,
        "role_embeddings": role_embeddings,
        "positional_embeddings": positional_embeddings,
        "accent_embedding": accent_embedding
    }
    np.savez(os.path.join(args.output_dir, "embeddings"), **embeddings)
    if model.acoustic_model.is_adaptive:
        gap_embedding = model.acoustic_model.gap_embedding.weight.data.numpy().reshape(-1)
        np.savez(os.path.join(args.output_dir, "gap_embedding"), gap_embedding=gap_embedding)


if __name__ == "__main__":
    main()
