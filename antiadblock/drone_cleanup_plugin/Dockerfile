FROM python:2.7-alpine

COPY "requirements.txt" "/"

RUN ["pip", "install", "-r", "requirements.txt"]

COPY "plugin/" "/plugin/"

ENTRYPOINT [ "python", "/plugin/cleanup_qloud_orpahned_items.py" ]
