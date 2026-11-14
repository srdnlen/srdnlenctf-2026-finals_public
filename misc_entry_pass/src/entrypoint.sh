#!/bin/sh
set -e

TEX_HOME="${TEX_HOME:-/opt/service/texhome}"
SCRATCH_ROOT="${SCRATCH_ROOT:-/opt/service/scratch}"
UPLOAD_DIR="${UPLOAD_DIR:-/opt/service/uploads}"

mkdir -p "$TEX_HOME" "$SCRATCH_ROOT" "$UPLOAD_DIR"
chmod 755 "$TEX_HOME"
chmod 1777 "$SCRATCH_ROOT"
chmod 755 "$UPLOAD_DIR"

rm -f /flag.txt
if [ -z "${FLAG}" ]; then
    FLAG='srdnlen{fake_flag}: Contact the organizers if you see this flag in remote instances.'
fi
printf '%s\n' "${FLAG}" > /flag.txt
chmod 400 /flag.txt
chown texsandbox:texsandbox /flag.txt

export SECRET_KEY="${SECRET_KEY:-$(python3 -c 'import secrets; print(secrets.token_hex(32))')}"

exec "$@"
