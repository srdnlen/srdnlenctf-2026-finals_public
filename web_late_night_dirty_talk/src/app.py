import hashlib
import os
import sqlite3

from flask import Flask, Response, jsonify, render_template, request
from gunicorn.dirty import DirtyError, get_dirty_client


DB = os.environ.get("DB_PATH", "/tmp/challenge.db")
DIRTY_APP = "snapshot_worker:Jobs"
app = Flask(__name__)


def connect():
    return sqlite3.connect(DB, timeout=10)


def init_db():
    flag = os.environ["FLAG"]
    with connect() as db:
        db.executescript("""
            DROP TABLE IF EXISTS documents;
            CREATE TABLE documents (
                id INTEGER PRIMARY KEY,
                owner TEXT NOT NULL,
                name TEXT NOT NULL,
                body BLOB NOT NULL
            );
        """)
        db.execute(
            "INSERT INTO documents(owner, name, body) VALUES ('admin', 'flag.txt', ?)",
            (flag.encode(),),
        )


@app.get("/")
def index():
    return render_template("index.html")


@app.post("/upload")
def upload():
    name = request.args.get("name", "document.bin")[:255]
    body = bytearray()
    with connect() as db:
        cursor = db.execute(
            "INSERT INTO documents(owner, name, body) VALUES ('guest', ?, X'')",
            (name,),
        )
        while chunk := request.stream.read(4096):
            body.extend(chunk)
        db.execute(
            "UPDATE documents SET body=? WHERE id=? AND owner='guest'",
            (body, cursor.lastrowid),
        )
    return jsonify(id=cursor.lastrowid, name=name, received=len(body))


@app.get("/integrity")
def integrity():
    digest = hashlib.sha256()
    try:
        for chunk in get_dirty_client().stream(DIRTY_APP, "snapshot"):
            digest.update(chunk)
    except DirtyError as error:
        return jsonify(error=str(error)), 504
    return jsonify(sha256=digest.hexdigest())


@app.get("/preview")
def preview():
    upload_id = request.args.get("id", type=int)
    return Response(
        get_dirty_client().stream(DIRTY_APP, "preview", upload_id),
        content_type="text/plain; charset=utf-8",
    )
