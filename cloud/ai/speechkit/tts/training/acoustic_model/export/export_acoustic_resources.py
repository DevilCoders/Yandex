import argparse
import os

import sys
from os.path import abspath, dirname

# enabling modules discovery from global entrypoint
sys.path.append(abspath(dirname(__file__) + "/../../"))

from acoustic_model.data.text_processor import get_text_processor_by_lang
from acoustic_model import AcousticTrainModule

import numpy as np
import torch


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=str, required=True)
    parser.add_argument("--output-dir", type=str, default="resources")
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

    token_embeddings = model.embedding.weight.data.numpy()
    speaker_embeddings = model.speaker_embedding.weight.data.numpy()
    role_embeddings = model.role_embedding.weight.data.numpy()
    positional_embeddings = model.positional_encoding._embeddings.squeeze(0).data.numpy()
    accent_embedding = model.accent_embedding.weight.data[1].numpy()
    embeddings = {
        "token_embeddings": token_embeddings,
        "speaker_embeddings": speaker_embeddings,
        "role_embeddings": role_embeddings,
        "positional_embeddings": positional_embeddings,
        "accent_embedding": accent_embedding
    }
    np.savez(os.path.join(args.output_dir, "embeddings"), **embeddings)
    if model.is_adaptive:
        gap_embedding = model.gap_embedding.weight.data.numpy().reshape(-1)
        np.savez(os.path.join(args.output_dir, "gap_embedding"), gap_embedding=gap_embedding)


if __name__ == "__main__":
    main()
