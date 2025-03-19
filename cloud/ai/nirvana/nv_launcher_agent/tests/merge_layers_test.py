import os
import tarfile

from cloud.ai.nirvana.nv_launcher_agent.lib.process.base_process import ProcessStatus
from cloud.ai.nirvana.nv_launcher_agent.lib.process.cli_process import CliProcess
from cloud.ai.nirvana.nv_launcher_agent.lib.helpers import prepare_archive


class TestCliProcess:
    def test_merge(self):
        cwd = os.getcwd()
        with open(cwd + "/a.txt", "w") as f:
            f.write("hello from ./a.txt")

        with open(cwd + "/b.txt", "w") as f:
            f.write("hello from ./b.txt")

        with open(cwd + "/c.txt", "w") as f:
            f.write("hello from ./c.txt")

        tar_ab = tarfile.open(cwd + "/ab.tar.gz", "w")
        for f in [cwd + "/a.txt", cwd + "/b.txt"]:
            tar_ab.add(f, f.split("/")[-1])
        tar_ab.close()

        tar_bc = tarfile.open(cwd + "/bc.tar.gz", "w")
        for f in [cwd + "/b.txt", cwd + "/c.txt"]:
            tar_bc.add(f, f.split("/")[-1])
        tar_bc.close()

        layers = [cwd + '/ab.tar.gz', cwd + '/bc.tar.gz']

        merge_args, _ = prepare_archive(layers, cwd)
        print(merge_args)
        import_image_process = CliProcess(
            name='merge_images',
            args=merge_args,
            log_dir=".",
            shell=True
        )
        import_image_process.start()
        import_image_process.wait()
        assert import_image_process.status == ProcessStatus.COMPLETED

        files = {"a.txt": 0, "b.txt": 0, "c.txt": 0}
        for file in os.listdir(cwd + "/untarred_layers"):
            print(file)
            files[file] += 1

        assert files["a.txt"] == 1
        assert files["b.txt"] == 1
        assert files["c.txt"] == 1
        assert sum(list(files.values())) == 3

        tar_res = tarfile.open(cwd + "/merged_layers.tar.gz")
        for member in tar_res.getmembers():
            if member.name == ".":
                continue
            f = tar_res.extractfile(member)

            content = f.read().decode('ascii')
            assert content == f'hello from {member.name}'

        tar_res.close()
