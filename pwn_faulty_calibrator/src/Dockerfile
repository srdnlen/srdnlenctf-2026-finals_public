FROM ubuntu:25.10@sha256:7cc5e35f6567ee8c66d2abb4aab0fd866669e6207c237c3a8f0947a5c7f17092

RUN apt-get clean
RUN apt-get update && apt-get install -y \
        cowsay \
        socat \
    && apt-get clean

RUN rm -rf /var/lib/apt/lists/*

RUN useradd ctf

WORKDIR /app

COPY --chown=root:ctf --chmod=050 ./build/faulty .
COPY --chown=root:ctf --chmod=040 ./flag.txt .

USER ctf

EXPOSE 2060

ENTRYPOINT ["socat", "-t", "300", "-T", "30", "TCP-LISTEN:2060,reuseaddr,nodelay,fork", "EXEC:/app/faulty,stderr"]