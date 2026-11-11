"""BankGuard - fraud case desk (Streamlit)."""
import html
import inspect
from pathlib import Path

import pandas as pd
import streamlit as st

st.set_page_config(page_title="BankGuard", page_icon="\U0001F6E1\ufe0f", layout="wide")

from bg_browse import browse
from bg_charts import account_chart, daily_chart, reason_chart, risk_chart
from bg_data import prepare, read_csv, run_demo_detector, score_with_detector
from bg_queue import queue
from bg_theme import ALERT_LEVELS, inject

STRETCH = {"width": "stretch"} if "width" in inspect.signature(st.button).parameters else {"use_container_width": True}
CFG = {"displayModeBar": False}
inject()

with st.sidebar:
    st.markdown("### Data")
    if st.button("Run detector on demo data", **STRETCH):
        try:
            st.session_state["demo"] = run_demo_detector()
        except NotImplementedError:
            st.warning("Connect run_demo_detector() in bg_data.py to your C++ detector.")
    upload = st.file_uploader("Transactions CSV", type="csv", label_visibility="collapsed")
    with st.expander("Accepted formats"):
        st.markdown("**Raw file:** scored by the detector. Needs account_id, timestamp, amount.\n\n"
                    "**Scored file:** shown as it is.")

raw, source = None, ""
if upload is not None:
    raw, source = read_csv(upload.getvalue()), upload.name
elif st.session_state.get("demo") is not None:
    raw, source = st.session_state["demo"], "Demo data"
else:
    found = next(iter(Path(".").rglob("scored_transactions.csv")), None)
    if found:
        raw, source = pd.read_csv(found), found.name

data = prepare(raw) if raw is not None else None
if raw is not None and data is None:
    try:
        data = prepare(score_with_detector(raw))
    except NotImplementedError:
        st.error("This file has no risk scores yet. Connect score_with_detector() in bg_data.py.")
        st.stop()

if data is None:
    st.markdown('<div class="band"><div class="brand">BankGuard</div><div class="tag">Upload a transactions CSV '
                'or run the demo from the sidebar to start.</div></div>', unsafe_allow_html=True)
    st.stop()

total = len(data)
alerts = int(data["level"].isin(ALERT_LEVELS).sum())
high = int(data["level"].isin(["CRITICAL", "HIGH"]).sum())

st.markdown(
    f'<div class="band"><div class="brand">BankGuard</div>'
    f'<div class="tag">Fraud case desk. {html.escape(source)} is loaded and scored.</div></div>'
    f'<div class="strip"><div><span>{total:,}</span>transactions</div>'
    f'<div class="hot"><span>{alerts:,}</span>alerts to review ({alerts / total:.1%})</div>'
    f'<div><span>{high:,}</span>high and critical</div>'
    f'<div><span>{data["score"].mean():.2f}</span>average risk score</div></div>', unsafe_allow_html=True)

tab1, tab2, tab3 = st.tabs(["Review queue", "Patterns", "All transactions"])
with tab1:
    queue(data)
with tab2:
    a, b = st.columns(2, gap="large")
    with a:
        st.markdown('<div class="h">Flagged transactions by level</div>', unsafe_allow_html=True)
        st.plotly_chart(risk_chart(data), **STRETCH, config=CFG)
    with b:
        st.markdown('<div class="h">Why transactions were flagged</div>', unsafe_allow_html=True)
        st.plotly_chart(reason_chart(data), **STRETCH, config=CFG)
    c, d = st.columns(2, gap="large")
    with c:
        st.markdown('<div class="h">Accounts with the most alerts</div>', unsafe_allow_html=True)
        st.plotly_chart(account_chart(data), **STRETCH, config=CFG)
    with d:
        if data["time"].notna().any() and data["time"].dt.date.nunique() > 1:
            st.markdown('<div class="h">Alerts per day</div>', unsafe_allow_html=True)
            st.plotly_chart(daily_chart(data), **STRETCH, config=CFG)
with tab3:
    browse(data, total)
