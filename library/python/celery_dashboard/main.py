import uvicorn

from library.python.celery_dashboard.api.app import app  # noqa


def main():
    uvicorn.run(
        'library.python.celery_dashboard.main:app',
        host='::',
        port=11000,
    )


if __name__ == "__main__":
    main()
