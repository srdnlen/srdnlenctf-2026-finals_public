bind = "0.0.0.0:8000"
workers = 2

dirty_apps = ["snapshot_worker:Jobs"]
dirty_workers = 1
dirty_timeout = 5


def on_starting(server):
    from app import init_db

    init_db()
