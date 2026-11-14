import re

MAX_NAME = 48
MAX_NICK = 32
MAX_TEAM = 48
MAX_COUNTRY = 24
MAX_STAFF_TITLE = 48
MAX_TALK_TITLE = 120
MAX_TALK_ABSTRACT = 600
MAX_DOWNLOAD_NAME = 64

DEFAULT_PASS_FILENAME = "sardinia-cyber-camp-badge.pdf"

PLAYER_BADGE_ROLES = ("Player", "Team captain")
STAFF_AREAS = ("Operations", "Infrastructure", "Registration", "Media", "Welcome desk")

ACCOUNT_ROLES = ("player", "speaker", "staff")

_LATEX_ESCAPES = {
    "\\": r"\textbackslash{}", "{": r"\{", "}": r"\}", "$": r"\$", "&": r"\&", "#": r"\#", "_": r"\_", "%": r"\%", "~": r"\textasciitilde{}", "^": r"\textasciicircum{}",
}

class ValidationError(Exception):
    pass

def latex_escape(s: str) -> str:
    return "".join(_LATEX_ESCAPES.get(ch, ch) for ch in s)

def _reject_control_chars(s: str, field: str) -> None:
    for ch in s:
        if ch in ("\n", "\t"):
            continue
        if ord(ch) < 0x20 or ord(ch) == 0x7f:
            raise ValidationError(f"{field}: control characters are not allowed")

def validate_text(value: str, field: str, maxlen: int, *, required: bool) -> str:
    value = value.strip()
    if not value:
        if required:
            raise ValidationError(f"{field}: must not be empty")
        return ""
    if len(value) > maxlen:
        raise ValidationError(f"{field}: max {maxlen} characters")
    _reject_control_chars(value, field)
    return value

def validate_player_role(role: str) -> str:
    if role not in PLAYER_BADGE_ROLES:
        raise ValidationError(f'role: must be one of {", ".join(PLAYER_BADGE_ROLES)}')
    return role

def validate_staff_area(area: str) -> str:
    if area not in STAFF_AREAS:
        raise ValidationError(f'area: must be one of {", ".join(STAFF_AREAS)}')
    return area

_DL_SAFE_RE = re.compile(r"[^A-Za-z0-9 ._-]+")

def validate_download_name(name: str) -> str:
    name = (name or "").strip().replace("\\", "/").rsplit("/", 1)[-1]
    name = _DL_SAFE_RE.sub("_", name)
    name = re.sub(r"(?:\.pdf)+$", "", name, flags=re.IGNORECASE)
    name = name.strip(" ._")[:MAX_DOWNLOAD_NAME - 4]
    return (name or "pass") + ".pdf"

_TITLE_FORBIDDEN = (
    "input", "include", "inputiffileexists", "lstinputlisting",
    "verbatiminput", "bverbatiminput", "lverbatiminput",
    "openin", "openout", "newread", "immediate", "special",
    "usepackage", "requirepackage",
    "csname", "endcsname", "expandafter", "scantokens", "noexpand", "catcode",
    "flag",
)
_TITLE_FORBIDDEN_RE = re.compile("|".join(_TITLE_FORBIDDEN), re.IGNORECASE)

def validate_talk_title(title: str) -> str:
    title = validate_text(title, "talk title", MAX_TALK_TITLE, required=False)
    if "\\" in title:
        raise ValidationError("talk title: backslash is not allowed")
    if "{" in title or "}" in title:
        raise ValidationError("talk title: braces are not allowed")
    if _TITLE_FORBIDDEN_RE.search(title):
        raise ValidationError("talk title: that reserved word is not allowed here")
    return title

def validate_talk_abstract(abstract: str) -> str:
    return validate_text(abstract, "talk abstract", MAX_TALK_ABSTRACT, required=False)

def expand_abstract_tokens(abstract: str, *, name: str, nickname: str, year: str) -> str:
    return (abstract.replace("{{name}}", name).replace("{{nickname}}", nickname).replace("{{year}}", year))
