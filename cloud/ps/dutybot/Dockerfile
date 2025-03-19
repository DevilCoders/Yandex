FROM cr.yandex/mirror/python:3.8-slim-bullseye

# Keeps Python from generating .pyc files in the container
ENV PYTHONDONTWRITEBYTECODE 1

# Turns off buffering for easier container logging
ENV PYTHONUNBUFFERED 1

# Install psycopg2-binary dependencies
RUN apt-get update \
    && apt-get -y install libpq-dev gcc

# Install pip requirements
ADD requirements.txt .
RUN python -m pip install --no-cache-dir -r requirements.txt

# Setting MSK timezone
RUN rm -f /etc/localtime; ln -s /usr/share/zoneinfo/Europe/Moscow /etc/localtime

WORKDIR /app
ADD . /app

RUN (echo ; cat /app/YandexInternalRootCA.pem) >> /usr/local/lib/python3.8/site-packages/certifi/cacert.pem

# Switching to a non-root user, please refer to https://aka.ms/vscode-docker-python-user-rights
RUN useradd appuser && chown -R appuser /app
USER appuser

# During debugging, this entry point will be overridden. For more information, please refer to https://aka.ms/vscode-docker-python-debug
CMD ["python", "bot.py"]
