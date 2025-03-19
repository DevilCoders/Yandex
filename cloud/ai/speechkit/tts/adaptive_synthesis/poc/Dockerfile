FROM nvidia/cuda:11.0-runtime-ubuntu16.04

RUN apt-get update && apt install -y software-properties-common
RUN add-apt-repository ppa:deadsnakes/ppa

RUN apt-get update && apt-get install -y python3.8 python3.8-distutils python3-setuptools sox ffmpeg llvm-8
RUN python3.8 -m easy_install pip
RUN python3.8 -m pip install --upgrade pip setuptools && python3.8 -m pip install llvmlite==0.31.0
RUN python3.8 -m pip install google protobuf grpcio grpcio-reflection grpcio-health-checking numpy scipy soundfile librosa sox pydub torch tqdm

ADD . /

CMD ["/usr/bin/python3.8", "server.py", "--vocoder", "/vocoder", "--tacotron", "/tacotron", "--lingware", "/lingware", "--preprocessor", "/preprocessor"]