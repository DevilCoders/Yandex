import argparse
import json
import os
import sys
import traceback

import nirvana.job_context as nv

from train_acoustic_model import main


def exit_with_error(message, code=1):
    sys.stderr.write("Error: {__message}\n".format(__message=message))
    sys.exit(code)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--local_rank", type=int, default=None)
    args = parser.parse_args()

    try:
        job_context = nv.context()
        inputs = job_context.get_inputs()
        outputs = job_context.get_outputs()
        parameters = job_context.get_parameters()

        args.data_config = inputs.get("data")
        args.model_config = inputs.get("model")
        args.optimizer_config = json.loads(parameters["optimizer-config"])
        args.speakers = inputs.get("speakers")
        args.text_processor_config = "acoustic_model/configs/text_processor.json"
        args.adaptive_mask_config = inputs.get("adaptive_mask") if inputs.has("adaptive_mask") else None

        args.checkpoint_dir = "checkpoint"
        args.logs_dir = "tensorboard"
        args.nirvana_checkpoint_path = outputs.get("checkpoint")
        args.nirvana_logs_path = outputs.get("tensorboard")
        args.previous_checkpoint_path = inputs.get("checkpoint") if inputs.has("checkpoint") else None

        args.num_steps = parameters["num-steps"]
        args.log_interval = parameters["log-interval"]
        args.val_interval = parameters["val-interval"]
        args.checkpoint_interval = parameters["checkpoint-interval"]

        args.lang = parameters["lang"]

        args.device = "cuda"
        args.use_amp = True
        args.num_workers = parameters["num-workers"]
        args.seed = parameters["seed"]

        os.environ["YT_TOKEN"] = parameters["yt-token"]

        with open(outputs.get("tensorboard_url"), "w") as f:
            instance_id = job_context.get_meta().get_workflow_instance_uid()
            block_code = job_context.get_meta().get_block_code()
            f.write(
                f"<a href='https://tensorboard.nirvana.yandex-team.ru/api/board/createFromNirvana"
                f"?instanceId={instance_id}&blockCode={block_code}&endpointName=tensorboard'>tensorboard</a>"
            )

        main(args)

    except Exception as e:
        exit_with_error(f"Crashed with {e}\n{traceback.format_exc()}", 1)
