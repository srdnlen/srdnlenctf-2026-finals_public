FROM python:3.14.7-slim

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app.py snapshot_worker.py gunicorn.conf.py ./
COPY templates ./templates
RUN useradd --create-home patriziofficial && chown -R patriziofficial:patriziofficial /app

USER patriziofficial
ENV DB_PATH=/tmp/challenge.db

CMD ["gunicorn", "-c", "gunicorn.conf.py", "app:app"]
