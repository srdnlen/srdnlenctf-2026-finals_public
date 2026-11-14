import functools
import io
import logging
import os
import re
import secrets

from flask import (
    Flask, abort, g, redirect, render_template,
    request, send_file, session, url_for
)

import db
import images
import sanitize
import sandbox

logging.basicConfig(level=logging.INFO)
log = logging.getLogger("badge")

app = Flask(__name__, template_folder="web/templates", static_folder="web/static")

app.secret_key = os.environ.get("SECRET_KEY") or secrets.token_hex(32)
app.config.update(
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE="Lax",
    MAX_CONTENT_LENGTH=4*1024*1024,
)

db.init_db()

USERNAME_MIN, USERNAME_MAX = 3, 32
PASSWORD_MIN, PASSWORD_MAX = 8, 32

REGISTERABLE_ROLES = ("player",)

_ACCT_FIELD_RE = re.compile(r"^account\[([A-Za-z_]+)\]$")

def _account_params(form) -> dict:
    return {m.group(1): v for k, v in form.items() if (m := _ACCT_FIELD_RE.match(k))}

def _check_username(u: str) -> str:
    u = u.strip()
    if not (USERNAME_MIN <= len(u) <= USERNAME_MAX):
        raise ValueError(f"Username must be {USERNAME_MIN}-{USERNAME_MAX} characters.")
    if not all(c.isalnum() or c in "_-." for c in u):
        raise ValueError("Username may use letters, digits, and _ - . only.")
    return u

def _check_password(p: str) -> str:
    if not (PASSWORD_MIN <= len(p) <= PASSWORD_MAX):
        raise ValueError(f"Password must be {PASSWORD_MIN}-{PASSWORD_MAX} characters.")
    return p

@app.before_request
def _load_user():
    g.user = None
    uid = session.get("uid")
    if uid is not None:
        g.user = db.get_user_by_id(uid)
        if g.user is None:
            session.clear()

def login_required(view):
    @functools.wraps(view)
    def wrapped(*args, **kwargs):
        if g.user is None:
            return redirect(url_for("login_form", next=request.path))
        return view(*args, **kwargs)
    return wrapped

@app.get("/register")
def register_form():
    if g.user is not None:
        return redirect(url_for("index"))
    return render_template("register.html", active="register", error=None, username="", first_name="", last_name="")

@app.post("/register")
def register():
    username = request.form.get("username", "")
    first_name = request.form.get("first_name", "")
    last_name = request.form.get("last_name", "")
    password = request.form.get("password", "")
    confirm = request.form.get("confirm", "")

    def fail(msg):
        return render_template("register.html", active="register", error=msg, username=username.strip(), first_name=first_name.strip(), last_name=last_name.strip()), 400

    try:
        username = _check_username(username)
        first_name = sanitize.validate_text(first_name, "first name", sanitize.MAX_NAME, required=True)
        last_name = sanitize.validate_text(last_name, "last name", sanitize.MAX_NAME, required=True)
        _check_password(password)
    except (ValueError, sanitize.ValidationError) as e:
        return fail(str(e))
    if password != confirm:
        return fail("Passwords do not match.")

    requested_role = request.form.get("role", "player").strip().lower()
    role = requested_role if requested_role in REGISTERABLE_ROLES else "player"
    try:
        uid = db.create_user(username, password, role, first_name, last_name)
    except db.DuplicateUser:
        return fail("That username is already taken.")

    session.clear()
    session["uid"] = uid
    return redirect(url_for("index"))

@app.get("/login")
def login_form():
    if g.user is not None:
        return redirect(url_for("index"))
    return render_template("login.html", active="login", error=None, username="")

@app.post("/login")
def login():
    username = request.form.get("username", "").strip()
    password = request.form.get("password", "")
    row = db.verify_credentials(username, password)
    if row is None:
        return render_template("login.html", active="login", error="Invalid username or password.", username=username), 401
    session.clear()
    session["uid"] = row["id"]
    nxt = request.args.get("next", "")
    if nxt.startswith("/") and not nxt.startswith("//"):
        return redirect(nxt)
    return redirect(url_for("index"))

@app.post("/logout")
def logout():
    session.clear()
    return redirect(url_for("login_form"))

def _render_profile(user, *, error=None, saved=False, overrides=None, code=200):
    o = overrides or {}
    html = render_template(
        "profile.html", active="profile", error=error, saved=saved,
        username=user["username"], nickname=user["nickname"], role=user["role"],
        first_name=o.get("first_name", user["first_name"]),
        last_name=o.get("last_name", user["last_name"]),
        country=o.get("country", user["country"]),
        team=o.get("team", user["team"]),
        has_photo=images.has_photo(user["id"]),
    )
    return html, code

@app.get("/profile")
@login_required
def profile_form():
    return _render_profile(g.user)[0]

@app.post("/profile")
@login_required
def profile_update():
    uid = g.user["id"]
    acct = _account_params(request.form)

    try:
        fields = {
            "first_name": sanitize.validate_text(acct.get("first_name", ""), "first name", sanitize.MAX_NAME, required=False),
            "last_name": sanitize.validate_text(acct.get("last_name", ""), "last name", sanitize.MAX_NAME, required=False),
            "country": sanitize.validate_text(acct.get("country", ""), "country", sanitize.MAX_COUNTRY, required=False),
            "team": sanitize.validate_text(acct.get("team", ""), "team", sanitize.MAX_TEAM, required=False),
        }
    except sanitize.ValidationError as e:
        return _render_profile(g.user, error=str(e), overrides=acct, code=400)

    db.update_profile(uid, {**acct, **fields})
    file = request.files.get("photo")
    if file and file.filename:
        try:
            name = images.save_profile_photo(uid, file)
            db.set_photo(uid, name)
        except images.PhotoError as e:
            fresh = db.get_user_by_id(uid)
            return _render_profile(fresh, error=f"Photo: {e}", overrides=acct, code=400)

    fresh = db.get_user_by_id(uid)
    return _render_profile(fresh, saved=True)[0]

@app.get("/me/photo")
@login_required
def my_photo():
    p = images.photo_path(g.user["id"])
    if not p.is_file():
        abort(404)
    return send_file(str(p), mimetype="image/png")

CHOICES = dict(
    player_roles=sanitize.PLAYER_BADGE_ROLES,
    staff_areas=sanitize.STAFF_AREAS,
)

_BADGE_KEYS = ("first_name", "last_name", "nickname", "country", "team", "role", "talk_title", "talk_abstract", "staff_area", "staff_title")

def _blank_badge() -> dict:
    return {k: "" for k in _BADGE_KEYS}

def _validate_identity(form: dict) -> dict:
    return {
        "first_name": sanitize.validate_text(form.get("first_name", ""), "first name", sanitize.MAX_NAME, required=True),
        "last_name": sanitize.validate_text(form.get("last_name", ""), "last name", sanitize.MAX_NAME, required=True),
        "nickname": sanitize.validate_text(form.get("nickname", ""), "nickname", sanitize.MAX_NICK, required=True),
        "country": sanitize.validate_text(form.get("country", ""), "country", sanitize.MAX_COUNTRY, required=False),
    }

def _validate_for_role(role: str, form: dict) -> dict:
    data = _blank_badge()
    data.update(_validate_identity(form))

    if role == "player":
        data["role"] = sanitize.validate_player_role(form.get("badge_role", ""))
        data["team"] = sanitize.validate_text(form.get("team", ""), "team", sanitize.MAX_TEAM, required=False)
    elif role == "speaker":
        data["role"] = "Speaker"
        data["talk_title"] = sanitize.validate_talk_title(form.get("talk_title", ""))
        abstract = sanitize.validate_talk_abstract(form.get("talk_abstract", ""))
        data["talk_abstract"] = sanitize.expand_abstract_tokens(abstract, name=f"{data['first_name']} {data['last_name']}".strip(), nickname=data["nickname"], year="2026")
    elif role == "staff":
        data["role"] = "Staff"
        data["staff_area"] = sanitize.validate_staff_area(form.get("staff_area", ""))
        data["staff_title"] = sanitize.validate_text(form.get("staff_title", ""), "staff title", sanitize.MAX_STAFF_TITLE, required=False)
    else:
        raise sanitize.ValidationError("unknown account role")
    return data

def _form_defaults(user) -> dict:
    role = user["role"]
    d = dict(_blank_badge(), **CHOICES, account_role=role)
    d["first_name"] = user["first_name"]
    d["last_name"] = user["last_name"]
    d["nickname"] = user["nickname"]
    d["country"] = user["country"]
    d["has_photo"] = images.has_photo(user["id"])
    if role == "player":
        d["badge_role"] = sanitize.PLAYER_BADGE_ROLES[0]
        d["team"] = user["team"]
    elif role == "staff":
        d["staff_area"] = sanitize.STAFF_AREAS[0]
        d["download_name"] = sanitize.DEFAULT_PASS_FILENAME
    return d

@app.get("/")
def index():
    if g.user is None:
        return render_template("landing.html", active="home")
    return render_template("index.html", error=None, active="pass", **_form_defaults(g.user))

@app.get("/faq")
def faq():
    return render_template("faq.html", active="faq")

@app.get("/code-of-conduct")
def code_of_conduct():
    return render_template("code_of_conduct.html", active="coc")

@app.post("/generate")
@login_required
def generate():
    user = g.user
    role = user["role"]
    form = dict(request.form)

    def rerender(error, code):
        echo = dict(_form_defaults(user), error=error, active="pass")
        for k in _BADGE_KEYS:
            if k in form:
                echo[k] = form.get(k, "")
        if role == "player":
            echo["badge_role"] = form.get("badge_role", echo["badge_role"])
        if role == "staff":
            echo["download_name"] = form.get("download_name", echo["download_name"])
        return render_template("index.html", **echo), code

    try:
        data = _validate_for_role(role, form)
    except sanitize.ValidationError as e:
        return rerender(str(e), 400)

    if images.has_photo(user["id"]):
        data["_photo_src"] = str(images.photo_path(user["id"]))

    download_name = sanitize.DEFAULT_PASS_FILENAME
    if role == "staff":
        download_name = sanitize.validate_download_name(form.get("download_name", ""))

    try:
        pdf_bytes = sandbox.compile_brochure(data)
    except sandbox.CompileError as e:
        log.warning("compile failed for %r: %s\n%s", data.get("nickname"), e, e.log_tail)
        return rerender("Could not build your pass, check your input.", 422)

    return send_file(
        io.BytesIO(pdf_bytes),
        mimetype="application/pdf",
        as_attachment=True,
        download_name=download_name,
    )

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000)
