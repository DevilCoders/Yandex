FROM tiangolo/uwsgi-nginx-flask:python3.6
RUN apt-get update
RUN apt-get install -y --force-yes bash nano
ENV STATIC_URL /static
ENV STATIC_PATH /var/www/app/static
ENV LISTEN_PORT [::]:8000
EXPOSE 8000
COPY ./app /app
COPY ./requirements.txt /var/www/requirements.txt
RUN pip install --upgrade pip
RUN pip install -r /var/www/requirements.txt
