"""HTML blocks shared by every page (pure Python, no Streamlit)."""
import html

from bg_content import FACTS, INCIDENTS, PIPELINE, TIPS_BANK, TIPS_CUSTOMER


def e(x):
    return html.escape(str(x))


def head(eyebrow, title, sub=""):
    p = f"<p>{e(sub)}</p>" if sub else ""
    return f'<div class="sec"><div class="eyebrow">{e(eyebrow)}</div><div class="h2">{e(title)}</div>{p}</div>'


def hero():
    return ('<div class="hero"><div><div class="eyebrow">Banking transaction anomaly detection</div>'
            '<div class="big">Catch suspicious money <span class="mark">before it moves on.</span></div>'
            '<div class="lead">BankGuard scores every transaction with rules, graph analysis and machine learning, '
            'then gives analysts a ranked list of what to review first.</div></div>'
            '<div><div class="tick"><small>Fraud reported by Indian banks, FY26</small><b>\u20b948,021 cr</b><span>across 10,114 cases (RBI)</span></div>'
            '<div class="tick alt"><small>Lost to cyber cheating in India, 2025</small><b>\u20b919,813 cr</b><span>about 2.18 million complaints (I4C)</span></div></div></div>')


def facts(items=FACTS):
    cards = "".join(f'<div class="fact"><b>{e(f["num"])}</b><h4>{e(f["title"])}</h4><p>{e(f["note"])}</p>'
                    f'<a href="{e(f["url"])}" target="_blank">{e(f["src"])}</a></div>' for f in items)
    return f'<div class="grid3">{cards}</div>'


def pipeline():
    cards = "".join(f'<div class="step"><i>{n}</i><h4>{e(t)}</h4><p>{e(b)}</p><small>{e(f)}</small></div>' for n, t, b, f in PIPELINE)
    return f'<div class="steps">{cards}</div>'


def incidents(items=INCIDENTS):
    rows = "".join(f'<div class="inc"><time>{e(i["date"])}</time><div><h4>{e(i["title"])}</h4><p>{e(i["body"])}</p>'
                   f'<a href="{e(i["url"])}" target="_blank">Source: {e(i["src"])}</a></div>'
                   f'<span class="tag">Detector signal: {e(i["signal"])}</span></div>' for i in items)
    return f"<div>{rows}</div>"


def tips():
    def box(title, items):
        li = "".join(f"<li>{e(t)}</li>" for t in items)
        return f'<div class="tipbox"><div class="h3">{e(title)}</div><ul>{li}</ul></div>'
    return (f'<div class="tips">{box("If you are a customer", TIPS_CUSTOMER)}{box("If you run a bank", TIPS_BANK)}</div>'
            '<div class="help">Lost money to a scam? Call 1930 or report at cybercrime.gov.in, then tell your bank immediately.</div>')


def tiles(items):
    cells = "".join(f'<div class="tile{" hot" if hot else ""}"><small>{e(label)}</small><b>{e(value)}</b></div>' for label, value, hot in items)
    return f'<div class="tiles">{cells}</div>'


def footer():
    return ('<div class="foot"><b>BankGuard</b><br>A student project on banking transaction anomaly detection. '
            'Facts and incidents were checked on 11 Nov 2026 and link to their sources. Verify figures before you cite them. '
            'The quick check on the Check page is a simplified rule scorer, not a bank-grade system.</div>')
