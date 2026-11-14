FROM cybersecnatlab/socaz:ubuntu-noble

RUN apt-get update && apt-get install -y python3-pip

COPY ./requirements.txt ./
RUN pip install -r requirements.txt --break-system-packages

COPY ./server.py /home/user/server
COPY ./ecc.py /home/user/ecc.py
COPY ./pedersen.py /home/user/pedersen.py
COPY ./transcript.py /home/user/transcript.py
COPY ./bulletproofs.py /home/user/bulletproofs.py

WORKDIR /home/user
CMD socaz --timeout 300 --flag-from-env FLAG --bind 1337 --cmd "python3 /home/user/server"
EXPOSE 1337
