import json


def update_file(path):
    with open(path) as f:
        data = json.load(f)

    for one in data:
        is_visible = len(one.get("queryFilters", [])) <= 1
        one["isVisible"] = is_visible

    with open(path, "w") as f:
        json.dump(data, f, indent=2, sort_keys=True)


def main():
    update_file("main_config.json")
    update_file("spam_config.json")


if __name__ == "__main__":
    main()
