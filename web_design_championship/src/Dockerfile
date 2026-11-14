FROM python:3.12-slim

WORKDIR /app

RUN apt-get update && apt-get install -y \
    chromium \
    chromium-driver \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir \
    flask \
    gunicorn \
    selenium

COPY app.py pow.py bot.py entrypoint.sh ./
COPY templates/ templates/

ENTRYPOINT ["sh", "./entrypoint.sh"]
