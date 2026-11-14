from __future__ import annotations

import os
import pwd
import resource
import shutil
import subprocess
import tempfile
from pathlib import Path

import jinja2

from sanitize import latex_escape

SERVICE_DIR = Path(__file__).resolve().parent
TEMPLATE_PATH = SERVICE_DIR / "latex" / "template.tex.jinja"
LOGO_SRC = SERVICE_DIR / "latex" / "assets" / "srdnlen-logo.png"

TEX_HOME = Path(os.environ.get("TEX_HOME", "/opt/service/texhome"))
SCRATCH_ROOT = Path(os.environ.get("SCRATCH_ROOT", "/opt/service/scratch"))

SANDBOX_USER = os.environ.get("SANDBOX_USER", "texsandbox")

COMPILE_TIMEOUT_SECS = 20
CPU_TIME_LIMIT_SECS = 15

MEM_LIMIT_BYTES = 512 * 1024 * 1024
NPROC_LIMIT = 64
FSIZE_LIMIT_BYTES = 32 * 1024 * 1024

class CompileError(Exception):
    def __init__(self, message: str, log_tail: str = ""):
        super().__init__(message)
        self.log_tail = log_tail

_jinja_env = jinja2.Environment(
    loader=jinja2.FileSystemLoader(str(TEMPLATE_PATH.parent)),
    block_start_string=r"\BLOCK{", block_end_string="}",
    variable_start_string=r"\VAR{", variable_end_string="}",
    comment_start_string=r"\#{", comment_end_string="}",
    trim_blocks=True, lstrip_blocks=True, autoescape=False,
)

_ESCAPE_FIELDS = ("first_name", "last_name", "nickname", "team", "role", "country", "staff_area", "staff_title", "talk_abstract")
_RAW_FIELDS = ("talk_title",)

def render_tex(data: dict) -> str:
    template = _jinja_env.get_template(TEMPLATE_PATH.name)
    ctx = {k: latex_escape(data.get(k, "")) for k in _ESCAPE_FIELDS}
    ctx.update({k: data.get(k, "") for k in _RAW_FIELDS})
    ctx["has_photo"] = bool(data.get("_photo_src"))
    return template.render(**ctx)

def _sandbox_uid_gid() -> tuple[int, int]:
    pw = pwd.getpwnam(SANDBOX_USER)
    return pw.pw_uid, pw.pw_gid

def _drop_privileges_and_limit():
    uid, gid = _sandbox_uid_gid()
    os.setgroups([])
    os.setgid(gid)
    os.setuid(uid)
    resource.setrlimit(resource.RLIMIT_CPU, (CPU_TIME_LIMIT_SECS, CPU_TIME_LIMIT_SECS))
    resource.setrlimit(resource.RLIMIT_AS, (MEM_LIMIT_BYTES, MEM_LIMIT_BYTES))
    resource.setrlimit(resource.RLIMIT_NPROC, (NPROC_LIMIT, NPROC_LIMIT))
    resource.setrlimit(resource.RLIMIT_FSIZE, (FSIZE_LIMIT_BYTES, FSIZE_LIMIT_BYTES))

def _run_pdflatex(cwd: Path) -> subprocess.CompletedProcess:
    env = {
        "HOME": str(TEX_HOME),
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
    }
    return subprocess.run(["pdflatex", "-shell-restricted", "-interaction=nonstopmode", "-halt-on-error", "main.tex"], cwd=str(cwd), env=env, preexec_fn=_drop_privileges_and_limit, timeout=COMPILE_TIMEOUT_SECS, capture_output=True, text=True)

def compile_brochure(data: dict) -> bytes:
    SCRATCH_ROOT.mkdir(parents=True, exist_ok=True)
    workdir = Path(tempfile.mkdtemp(prefix="badge-", dir=str(SCRATCH_ROOT)))
    try:
        os.chmod(workdir, 0o711)

        tex_source = render_tex(data)
        main_tex = workdir / "main.tex"
        main_tex.write_text(tex_source, encoding="utf-8")

        logo_copy = workdir / "srdnlen-logo.png"
        shutil.copyfile(LOGO_SRC, logo_copy)

        photo_src = data.get("_photo_src")
        avatar_copy = None
        if photo_src and Path(photo_src).is_file():
            avatar_copy = workdir / "avatar.png"
            shutil.copyfile(photo_src, avatar_copy)

        uid, gid = _sandbox_uid_gid()
        os.chown(main_tex, uid, gid)
        os.chown(logo_copy, uid, gid)
        if avatar_copy is not None:
            os.chown(avatar_copy, uid, gid)
        os.chown(workdir, uid, gid)

        try:
            last_result = _run_pdflatex(workdir)
        except subprocess.TimeoutExpired as e:
            raise CompileError("Compile timed out", log_tail=(e.stdout or "")[-4000:])

        pdf_path = workdir / "main.pdf"
        if last_result.returncode != 0 or not pdf_path.exists():
            raise CompileError("compile failed", log_tail=(last_result.stdout or "")[-4000:])
        return pdf_path.read_bytes()
    finally:
        shutil.rmtree(workdir, ignore_errors=True)
