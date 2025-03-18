import sys
import json
import re


def check_deps(content_path, resources, found_set):
    with open(content_path, 'r') as inp:
        content = inp.read()

    for m in re.finditer(r"/(captcha_smart|checkbox|advanced)[\.\w]*?\.(html|js|css)", content, flags=re.DOTALL):
        link = m.group()
        found_set.add(link)
        found = False
        for res in resources:
            if res.endswith(link):
                check_deps(res, resources, found_set)
                found = True
                break
        if not found:
            raise Exception(f"Could not found file '{link}' in resources. Probably you need to add it to ya.make")


if __name__ == "__main__":
    mode = sys.argv[1]
    static_versions_inc_file = sys.argv[2]
    static_version_list_inc_file = sys.argv[3]
    files = sys.argv[4:]

    with open(static_versions_inc_file, 'w') as out:
        for f in files:
            path = "/" + f.split('/')[-1]
            print(json.dumps(path) + ",", file=out)

    if mode == "external":
        captcha_js_files = [f for f in files if f.endswith('captcha.js')]
        assert len(captcha_js_files) >= 2, "Need at least 2 last captcha.js versions"
        for captcha_js_file in captcha_js_files:
            deps_set = set()
            check_deps(captcha_js_file, files, deps_set)
            assert len(deps_set) == 4, deps_set

        versions = sorted(map(lambda x: int(x.split('/')[-2]), captcha_js_files))
        with open(static_version_list_inc_file, 'w') as out:
            print(",\n".join(map(str, versions)), file=out)
    elif mode == "antirobot":
        root_html_files = [f for f in files if ".html" in f]
        assert len(root_html_files) >= 10
        for root_html_file in root_html_files:
            deps_set = set()
            check_deps(root_html_file, files, deps_set)
            assert len(deps_set) == 3, deps_set

        versions = sorted(map(lambda x: int(x.split('/')[-1].split('-')[0]), root_html_files))
        with open(static_version_list_inc_file, 'w') as out:
            print(",\n".join(map(str, versions)), file=out)
    else:
        assert False, f"Unknown mode {mode}"
