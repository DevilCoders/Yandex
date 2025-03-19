PACKAGE_NAME = "speechkit"


def main():
    from setuptools import setup

    setup(
        name=PACKAGE_NAME,
        version="0.4",
        packages=[
            "yandex/cloud/ai/tts/v3",
            "yandex/cloud/ai/stt/v3",
            "yandex/cloud/ai/stt/v2",
            "yandex/cloud/operation",
            "yandex/cloud/api",
            "yandex/cloud/api/tools",
            "speechkit",
            "speechkit/common",
            "speechkit/common/utils",
            "speechkit/stt",
            "speechkit/stt/azure",
            "speechkit/stt/yandex",
            "speechkit/tts",
            "google",
            "google/rpc"
        ],
        install_requires=["grpcio-tools", "boto3", "pydub"],
        # package_data={"stt_metrics": ["alignment_utils.so"]},
        author="cloud",
        author_email="tbd",
        description="SpeechKit package",
        keywords="speechkit",
        include_package_data=True
    )


if __name__ == "__main__":
    main()
