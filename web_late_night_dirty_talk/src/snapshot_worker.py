import os
import sqlite3

from gunicorn.dirty import DirtyApp


DB = os.environ.get("DB_PATH", "/tmp/challenge.db")


class Jobs(DirtyApp):
    def snapshot(self):
        with sqlite3.connect(DB, timeout=10) as db:
            db.execute("UPDATE documents SET name=name WHERE owner='admin'")
            rows = db.execute(
                "SELECT owner, name, body FROM documents ORDER BY id"
            ).fetchall()

        for owner, name, body in rows:
            yield f"{owner}/{name}:".encode() + body[:4096]
            yield b"\n[truncated]\n" if len(body) > 4096 else b"\n"

    def preview(self, upload_id=None):
        with sqlite3.connect(DB) as db:
            if upload_id is None:
                row = db.execute(
                    "SELECT id, name, body FROM documents "
                    "WHERE owner='guest' ORDER BY id DESC LIMIT 1"
                ).fetchone()
            else:
                row = db.execute(
                    "SELECT id, name, body FROM documents "
                    "WHERE id=? AND owner='guest'",
                    (upload_id,),
                ).fetchone()

        if row is None:
            yield b"No documents uploaded.\n"
            return

        document_id, name, body = row
        yield f"Document #{document_id}: {name} ({len(body)} bytes)\n\n".encode()
        yield body[:4096]
        if len(body) > 4096:
            yield b"\n\n[preview truncated]\n"
