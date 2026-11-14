from __future__ import annotations

import os
import sqlite3
import time
from pathlib import Path

from werkzeug.security import check_password_hash, generate_password_hash

SERVICE_DIR = Path(__file__).resolve().parent
DB_PATH = os.environ.get("USER_DB", str(SERVICE_DIR / "users.db"))

ACCOUNT_ROLES = ("player", "speaker", "staff")

PROTECTED_COLUMNS = {"id", "username", "nickname", "password_hash", "role", "created_at"}

_HASH_METHOD = "pbkdf2:sha256"
_DUMMY_HASH = generate_password_hash("dummy", method=_HASH_METHOD)

class DuplicateUser(Exception):
    pass

def _connect() -> sqlite3.Connection:
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA busy_timeout=5000;")
    return conn

def init_db() -> None:
    with _connect() as conn:
        conn.execute(
            """CREATE TABLE IF NOT EXISTS users (
                   id            INTEGER PRIMARY KEY AUTOINCREMENT,
                   username      TEXT UNIQUE NOT NULL COLLATE NOCASE,
                   password_hash TEXT NOT NULL,
                   role          TEXT NOT NULL,
                   created_at    INTEGER NOT NULL,
                   nickname      TEXT NOT NULL DEFAULT '',
                   first_name    TEXT NOT NULL DEFAULT '',
                   last_name     TEXT NOT NULL DEFAULT '',
                   country       TEXT NOT NULL DEFAULT '',
                   team          TEXT NOT NULL DEFAULT '',
                   photo         TEXT NOT NULL DEFAULT ''
               );"""
        )

def create_user(username: str, password: str, role: str, first_name: str = "", last_name: str = "") -> int:
    if role not in ACCOUNT_ROLES:
        raise ValueError(f"invalid role: {role!r}")
    pw_hash = generate_password_hash(password, method=_HASH_METHOD)
    try:
        with _connect() as conn:
            cur = conn.execute("INSERT INTO users (username, password_hash, role, nickname, first_name, last_name, created_at) VALUES (?, ?, ?, ?, ?, ?, ?);", (username, pw_hash, role, username, first_name, last_name, int(time.time())))
            return int(cur.lastrowid)
    except sqlite3.IntegrityError:
        raise DuplicateUser(username)

def get_user_by_id(uid: int) -> sqlite3.Row | None:
    with _connect() as conn:
        return conn.execute("SELECT * FROM users WHERE id = ?;", (uid,)).fetchone()

def get_user_by_name(username: str) -> sqlite3.Row | None:
    with _connect() as conn:
        return conn.execute("SELECT * FROM users WHERE username = ?;", (username,)).fetchone()

def verify_credentials(username: str, password: str) -> sqlite3.Row | None:
    row = get_user_by_name(username)
    if row is None:
        check_password_hash(_DUMMY_HASH, password)
        return None
    if check_password_hash(row["password_hash"], password):
        return row
    return None

def update_profile(uid: int, fields: dict) -> None:
    cols = [k for k in fields if k not in PROTECTED_COLUMNS]
    if not cols:
        return
    assignments = ", ".join(f"{c} = ?" for c in cols)
    values = [fields[c] for c in cols]
    with _connect() as conn:
        conn.execute(f"UPDATE users SET {assignments} WHERE id = ?;", (*values, uid))

def set_photo(uid: int, filename: str) -> None:
    with _connect() as conn:
        conn.execute("UPDATE users SET photo = ? WHERE id = ?;", (filename, uid))

def set_role(uid: int, role: str) -> None:
    with _connect() as conn:
        conn.execute("UPDATE users SET role = ? WHERE id = ?;", (role, uid))
