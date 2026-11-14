
FROM python:3.12-slim

WORKDIR /app

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

COPY requirements.txt .

RUN pip install --no-cache-dir -r requirements.txt

RUN apt-get update && apt-get install -y --no-install-recommends \
    fontforge \
    fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*

COPY . .

EXPOSE 5051

CMD ["python", "exploit32.2.py"]
