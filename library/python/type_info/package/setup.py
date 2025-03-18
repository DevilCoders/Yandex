PACKAGE_NAME = "yandex-type-info"


def main():
    from setuptools import setup
    setup(
        name=PACKAGE_NAME,
        version="0.0.6",
        packages=["yandex", "yandex.type_info"],
        package_data={"yandex.type_info": [
            "bindings.so",
        ]},
        author="yt",
        author_email="yt@yandex-team.ru",
        description="Common Yandex types",
        keywords="yandex types",
        include_package_data=True,
    )
if __name__ == "__main__":
    main()
