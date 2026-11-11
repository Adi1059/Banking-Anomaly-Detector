"""BankGuard - C++-powered banking anomaly review dashboard."""
from pathlib import Path
import html
import time

import pandas as pd
import streamlit as st

st.set_page_config(page_title="BankGuard | Anomaly Detection", page_icon="🛡️", layout="wide")

import engine
from bg_browse import browse
from bg_charts import account_chart, daily_chart, reason_chart, risk_chart
from bg_data import prepare, read_csv, run_demo_detector, score_with_detector
from bg_queue import queue
from bg_theme import ALERT_LEVELS, inject

inject()
ROOT = Path(__file__).resolve().parent
DEFAULT_FILE = ROOT / "cpp" / "data" / "scored_transactions.csv"

def show_engine_error(message):
    st.error("The C++ detector could not run.")
    st.code(str(message)[-2500:])

with st.sidebar:
    st.markdown("## BankGuard")
    st.caption("Banking transaction anomaly detection")
    st.markdown("---")
    if st.button("▶ Run C++ detector on demo data", use_container_width=True, type="primary"):
        with st.spinner("Building and running the C++ detector..."):
            ok, message, result = run_demo_detector()
        if ok:
            st.session_state["bankguard_data"] = result
            st.session_state["bankguard_source"] = "C++ demo detector"
            st.success("Demo analysis completed.")
        else:
            show_engine_error(message)
    uploaded = st.file_uploader("Upload transactions CSV", type=["csv"])
    st.markdown("**Raw CSV needs:** `account_id`, `timestamp`, `amount`")
    st.caption("Optional: receiver_id. Scored files may use risk_score / risk_level.")
    st.markdown("---")
    st.caption("Detection runs through the existing C++ backend in `cpp/`. The dashboard does not replace that engine.")

raw = None
source = ""
if uploaded is not None:
    try:
        raw = read_csv(uploaded.getvalue())
        source = uploaded.name
    except Exception as exc:
        st.error(f"Could not read this CSV: {exc}")
        st.stop()
elif st.session_state.get("bankguard_data") is not None:
    raw = st.session_state["bankguard_data"]
    source = st.session_state.get("bankguard_source", "Demo data")
elif DEFAULT_FILE.exists():
    try:
        raw = pd.read_csv(DEFAULT_FILE)
        source = "Project scored_transactions.csv"
    except Exception as exc:
        st.warning(f"Could not read the default scored file: {exc}")

if raw is not None:
    try:
        prepared = prepare(raw)
        if prepared is None:
            with st.spinner("Scoring raw transactions with the C++ detector..."):
                ok, message, scored = score_with_detector(raw)
            if not ok:
                show_engine_error(message)
                st.stop()
            prepared = prepare(scored)
            source += " · scored by C++"
        if prepared is None or prepared.empty:
            st.error("No usable transactions were found. Check the CSV columns and values.")
            st.stop()
    except Exception as exc:
        st.error(f"Could not prepare this dataset: {exc}")
        st.stop()
else:
    prepared = None

if prepared is None:
    st.markdown("""
    <div class="hero">
      <div class="eyebrow">Banking transaction anomaly detection</div>
      <h1>🛡️ BankGuard</h1>
      <p>Run the C++ detector on demo data or upload a transaction CSV to begin.</p>
    </div>
    """, unsafe_allow_html=True)
    st.info("No transaction data is loaded yet.")
    st.markdown("**Raw CSV columns:** `account_id`, `timestamp`, `amount` (and optionally `receiver_id`).")
    st.stop()

data = prepared
total = len(data)
alerts = int(data["level"].isin(ALERT_LEVELS).sum())
high_critical = int(data["level"].isin(["CRITICAL", "HIGH"]).sum())
alert_rate = alerts / total * 100 if total else 0

st.markdown(f"""
<div class="hero">
  <div class="eyebrow">Banking transaction anomaly detection</div>
  <h1>🛡️ BankGuard</h1>
  <p>Fraud case desk · {html.escape(source)}</p>
</div>
""", unsafe_allow_html=True)

st.caption(f"Data source: {source} · {total:,} transactions loaded")
k1, k2, k3, k4, k5 = st.columns(5)
k1.metric("Total transactions", f"{total:,}")
k2.metric("Alerts detected", f"{alerts:,}")
k3.metric("Alert rate", f"{alert_rate:.1f}%")
k4.metric("High / critical", f"{high_critical:,}")
k5.metric("Average risk score", f"{data['score'].mean():.2f}")

tab1, tab2, tab3 = st.tabs(["Review queue", "Patterns", "All transactions"])
with tab1:
    queue(data, source)
with tab2:
    left, right = st.columns(2, gap="large")
    with left:
        st.plotly_chart(risk_chart(data), use_container_width=True, config={"displayModeBar": False})
    with right:
        st.plotly_chart(reason_chart(data), use_container_width=True, config={"displayModeBar": False})
    left, right = st.columns(2, gap="large")
    with left:
        st.plotly_chart(account_chart(data), use_container_width=True, config={"displayModeBar": False})
    with right:
        if data["time"].notna().any():
            st.plotly_chart(daily_chart(data), use_container_width=True, config={"displayModeBar": False})
        else:
            st.info("Daily pattern chart needs usable timestamps.")
with tab3:
    browse(data, total)

st.markdown("---")
st.markdown('<p class="small-muted">BankGuard · Academic prototype · Risk scores are decision-support signals, not proof of fraud.</p>',
            unsafe_allow_html=True)
