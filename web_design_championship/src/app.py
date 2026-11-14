from flask import Flask, g, request, render_template, session, redirect
import base64, os, pow, secrets, threading
from bot import visit

app = Flask(__name__)
app.secret_key = os.environ["SECRET_KEY"]
FLAG = os.environ["FLAG"]

@app.before_request
def generate_csp_nonce():
    g.csp_nonce = secrets.token_urlsafe(16)

@app.after_request
def _apply_security_headers(response):
    nonce = g.csp_nonce
    response.headers["Content-Security-Policy"] = (
        "default-src 'unsafe-inline' * data:; "
        f"script-src 'nonce-{nonce}'; "
        "connect-src 'self'; "
        "form-action 'self'; "
        "base-uri 'none'; "
        "frame-ancestors 'none'; "
        "object-src 'none'"
    )
    response.headers["X-Content-Type-Options"] = "nosniff"
    response.headers["Referrer-Policy"] = "no-referrer"
    return response

def _pow_resource() -> str:
    if not session.get("pow"):
        session["pow"] = pow.new_resource()
    return session["pow"]

@app.route("/")
def index():
    css = ""
    css_b64 = request.args.get("css")
    if css_b64:
        try:
            css = base64.urlsafe_b64decode(css_b64).decode("utf-8")
        except Exception:
            css = "/* error decoding base64 */"
    secret = request.cookies.get("secret", "")
    return render_template(
        "index.html",
        css_b64=css_b64,
        css=css,
        secret=secret,
        resource=_pow_resource(),
        pow_bits=pow.BITS,
        nonce=g.csp_nonce
    )

@app.route("/set-cookie", methods=["POST"])
def set_cookie():
    value = request.form.get("secret", "")
    resp = redirect(f"/?css={request.args.get('css')}")
    resp.set_cookie("secret", value)
    return resp

@app.post("/report")
def report():
    body = request.get_json(silent=True) or {}
    css = request.form.get("css") or body.get("css", "")
    if css == "":
        return 'invalid css', 400
    pow_stamp = request.form.get("pow") or body.get("pow", "")
    resource = _pow_resource()
    if not isinstance(pow_stamp, str) or not pow.check(pow_stamp, resource):
        return 'invalid proof of work', 400
    # single use: rotate the challenge after a valid solution
    session["pow"] = pow.new_resource()
    try:
        threading.Thread(target=visit, args=(FLAG, "localhost:5050", css)).start()
    except Exception:
        return "internal error", 500
    return "thx", 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5050)