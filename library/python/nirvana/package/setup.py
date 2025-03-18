PACKAGE_NAME = "nirvana-context"


def main():
    import os
    from setuptools import setup

    install_requires = [
        "yandex-yt>=0.10.3",
    ]

    setup(
        name=PACKAGE_NAME,
        version=os.environ.get("YA_PACKAGE_VERSION"),
        packages=[
            "nirvana",
        ],
        install_requires=install_requires,
        author="nirvana",
        author_email="nirvana@yandex-team.ru",
        description="Nirvana library with context modules",
        keywords="yandex nirvana job-processor",
    )


if __name__ == "__main__":
    main()
