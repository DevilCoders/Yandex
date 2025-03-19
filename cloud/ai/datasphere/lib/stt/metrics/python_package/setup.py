PACKAGE_NAME = "stt-metrics"


def main():
    from setuptools import setup

    setup(
        name=PACKAGE_NAME,
        version="0.2",
        packages=["stt_metrics", "stt_metrics/normalizer_service"],
        install_requires=["pymorphy2", "grpcio-tools"],
        package_data={"stt_metrics": ["alignment_utils.so"]},
        author="eranik",
        author_email="eranik@yandex-team.ru",
        description="STT metrics package",
        keywords="stt metrics",
        include_package_data=True
    )


if __name__ == "__main__":
    main()
