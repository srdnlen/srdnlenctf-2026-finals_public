#!/bin/sh

export SECRET_KEY=$(cat /dev/urandom | base64 | head -c 30)

exec gunicorn \
    --bind 0.0.0.0:5050 \
    --workers 4 \
    --threads 2 \
    --timeout 120 \
    --capture-output \
    app:app