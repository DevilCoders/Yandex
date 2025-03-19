from python:3.8.6
RUN apt-get update && apt-get -y install libev-dev
RUN pip3 install --upgrade pip
COPY ./support_keeper/ ./
RUN pip3 install -r requirements.txt

# comment 
