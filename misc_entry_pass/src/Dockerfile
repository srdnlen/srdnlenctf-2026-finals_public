FROM texlive/texlive:latest-small

RUN tlmgr update --self && tlmgr install fancyvrb && kpsewhich fancyvrb.sty
RUN apt-get update && apt-get install -y --no-install-recommends python3-pip && rm -rf /var/lib/apt/lists/*
RUN useradd --system --create-home --home-dir /home/texsandbox --shell /usr/sbin/nologin texsandbox

WORKDIR /opt/service

COPY requirements.txt .
RUN pip3 install --no-cache-dir --break-system-packages -r requirements.txt

COPY sanitize.py sandbox.py db.py images.py app.py entrypoint.sh ./
COPY latex ./latex
COPY web ./web

RUN chmod +x entrypoint.sh

ENV TEX_HOME=/opt/service/texhome
ENV SCRATCH_ROOT=/opt/service/scratch
ENV SANDBOX_USER=texsandbox
ENV PYTHONPATH=/opt/service

EXPOSE 8000
ENTRYPOINT ["./entrypoint.sh"]
CMD ["gunicorn", "-b", "0.0.0.0:8000", "--workers", "2", "--worker-class", "sync", "--timeout", "30", "app:app"]
