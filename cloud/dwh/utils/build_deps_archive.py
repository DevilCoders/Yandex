import os
import re
import zipfile
import yt.wrapper as yt

ARC_ROOT = "/Users/soin08/arc/arcadia"


def extract_peer_dir(file):
    with open(file, "r") as f:
        text = f.read()
    matches = re.finditer('PEERDIR\((.*?)\)', text, flags=re.DOTALL)
    result = []
    for match in matches:
        modules = match.groups()[0]
        modules = modules.split("\n")
        result.extend([m.strip() for m in modules if not m.strip() == ""])
    return result


def find_deps_for_module(arc_module):
    result = set()

    def _get_deps(arc_module):
        ya_make_file = f"{ARC_ROOT}/{arc_module}/ya.make"
        if not os.path.isfile(ya_make_file):
            return
        deps = extract_peer_dir(ya_make_file)
        for dep in deps:
            if dep not in result:
                result.add(dep)
                _get_deps(dep)

    _get_deps(arc_module)
    return result


def find_deps(module_list):
    result = set()
    for dep in module_list:
        deps = find_deps_for_module(dep)
        result.update(deps)
    return result


def add_dep_to_archive(archive, arc_dep_folder, zip_prefix):
    folder = f"{ARC_ROOT}/{arc_dep_folder}"
    for root, dirs, files in os.walk(folder):
        for file in files:
            path = os.path.join(root, file)
            zip_path = path.replace(folder, f"{zip_prefix}/")
            print(f"Adding {path}")
            archive.write(path, zip_path)


def build_archive(deps_list, zip_path):
    archive = zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED)
    for dep in deps_list:
        module_name = os.path.basename(dep) if not dep.endswith("enum34") else "enum"
        folder = f"{dep}/{module_name}"
        zip_prefix = module_name
        add_dep_to_archive(archive, folder, zip_prefix)

    add_dep_to_archive(archive, "cloud/dwh/spark", "spark")
    # add_dep_to_archive(archive, "cloud/dwh/_contrib/yt", "yt")
    # add_dep_to_archive(archive, "cloud/dwh/_contrib/yt_yson_bindings", "yt_yson_bindings")


def upload_to_yt(local_path, yt_path):
    print(f"uploading archive to {yt_path}")
    yt.config['proxy']['url'] = YT_PROXY
    with open(local_path, "rb") as f:
        yt.write_file(yt_path, f)


PY_DEPS = ["cloud/dwh/_contrib"]
LOCAL_ZIP_PATH = "deps.zip"
YT_ZIP_PATH = "//home/cloud_analytics/dwh/spark/dependencies.zip"
YT_PROXY = 'hahn'


def main():
    deps = find_deps(PY_DEPS)
    #deps.update(PY_DEPS)
    #deps.remove('library/python/vault_client')
    #deps.remove('contrib/python/paramiko')
    #deps.remove('contrib/python/bcrypt')
    #deps.remove('contrib/python/six')
    print(deps)
    build_archive(deps, LOCAL_ZIP_PATH)
    upload_to_yt(LOCAL_ZIP_PATH, YT_ZIP_PATH)


#def main():
 #   print(find_deps_for_module("/contrib/python/cryptography"))


if __name__ == "__main__":
    main()

