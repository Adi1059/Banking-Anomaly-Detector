import re
from pathlib import Path

import pandas as pd
import plotly.express as px
import streamlit as st

import engine

st.set_page_config(page_title="BankGuard | Anomaly Detection", page_icon="🛡️", layout="wide")

LEVEL_ORDER = ["CRITICAL", "HIGH", "MEDIUM", "LOW", "NONE"]
LEVEL_COLORS = {"CRITICAL": "#8b1e3f", "HIGH": "#e05252", "MEDIUM": "#e7a23b",
                "LOW": "#4c83c3", "NONE": "#9aa6b2"}
ALERT_LEVELS = ["MEDIUM", "HIGH", "CRITICAL"]

# ---------------------------------------------------------------- styling
# Every colour is set explicitly (text AND background) so the page looks the
# same whether the user's Streamlit/VS Code theme is light or dark.
st.markdown("""
<style>
.stApp { background: #f5f7fb; }
.block-container { padding-top: 3.5rem; max-width: 1450px; }
.stApp, .stApp p, .stApp label, .stApp span, .stApp li,
.stApp h1, .stApp h2, .stApp h3, .stApp h4 { color: #1b2740; }
.hero { background: linear-gradient(110deg,#101d3a,#183c68); padding: 25px 28px;
        border-radius: 16px; margin-bottom: 18px; }
.stApp .hero h1, .stApp .hero h1 span { color: #ffffff !important; margin: 0; padding: 0; font-size: 30px; }
.stApp .hero p  { color: #d8e5f5 !important; margin: 7px 0 0 0; }
header[data-testid="stHeader"] { background: #f5f7fb; }
header[data-testid="stHeader"] * { color: #1b2740 !important; }
section[data-testid="stSidebar"] [data-testid="stFileUploaderDropzone"] { background: #f0f4fa; }
section[data-testid="stSidebar"] [data-testid="stFileUploaderDropzone"] * { color: #1b2740; }
section[data-testid="stSidebar"] button { background: #ffffff; border: 1px solid #cfdcf0; }
div[data-testid="stMetric"] { background: #ffffff; padding: 15px 18px;
        border: 1px solid #e7ebf2; border-radius: 12px; }
div[data-testid="stMetric"] * { color: #1b2740 !important; }
section[data-testid="stSidebar"] { background: #ffffff; border-right: 1px solid #e7ebf2; }
section[data-testid="stSidebar"] * { color: #1b2740; }
.small-muted { color: #68758a !important; font-size: 13px; }
.badge { display:inline-block; padding:3px 12px; border-radius:999px; color:#fff !important;
         font-weight:600; font-size:13px; }
.chip { display:inline-block; padding:4px 11px; margin:3px 6px 3px 0; border-radius:8px;
        background:#eaf1fb; border:1px solid #cfdcf0; color:#1b2740 !important; font-size:13px; }
</style>
""", unsafe_allow_html=True)

st.markdown("""
<div class="hero">
  <h1>🛡️ BankGuard</h1>
  <p>Banking Transaction Anomaly Detection · Rules + Graph + Isolation Forest + Hybrid Risk Scoring</p>
</div>
""", unsafe_allow_html=True)


# ---------------------------------------------------------------- helpers
def split_reasons(text: str) -> list[str]:
    """The C++ detector writes reasons as 'a; b; c' – accept ';' and ',' both."""
    return [p.strip() for p in re.split(r"[;,]", text or "") if p.strip()]


def pretty(reason: str) -> str:
    return reason.replace("_", " ").title()


def style_fig(fig):
    fig.update_layout(template="plotly_white", margin=dict(l=5, r=5, t=40, b=5),
                      plot_bgcolor="white", paper_bgcolor="white",
                      font=dict(color="#1b2740"), title_font=dict(color="#1b2740", size=15))
    fig.update_xaxes(tickfont=dict(color="#1b2740"), title_font=dict(color="#1b2740"),
                     gridcolor="#e7ebf2", linecolor="#cfd6e2")
    fig.update_yaxes(tickfont=dict(color="#1b2740"), title_font=dict(color="#1b2740"),
                     gridcolor="#e7ebf2", linecolor="#cfd6e2")
    return fig


# ---------------------------------------------------------------- data
default_file = Path(__file__).parent / "cpp" / "data" / "scored_transactions.csv"

st.sidebar.header("Data")
run_demo_clicked = st.sidebar.button("▶ Run the C++ detector on demo data", use_container_width=True)
uploaded = st.sidebar.file_uploader("Upload a transactions CSV (raw or already scored)", type=["csv"])
st.sidebar.markdown("---")
st.sidebar.markdown("**Raw file (the detector scores it for you)**")
st.sidebar.caption("account_id, timestamp, amount  (receiver_id is optional)")
st.sidebar.markdown("**Scored file (shown as it is)**")
st.sidebar.caption("id, sender, receiver, amount, timestamp, risk, level, reasons")
st.sidebar.caption("(tx_id, account_id, receiver_id, risk_score, risk_level are accepted too)")
st.sidebar.markdown("---")
st.sidebar.caption("All detection runs in the C++ backend (cpp/). This page only starts it and shows the result.")


@st.cache_data(show_spinner=False)
def load_csv(src):
    return pd.read_csv(src)


@st.cache_data(show_spinner=False)
def score_cached(raw):
    return engine.score_dataframe(raw)


try:
    if run_demo_clicked:
        with st.spinner("Running the C++ detector..."):
            ok, message, df = engine.run_demo()
        if not ok:
            st.error("The detector could not run:\n\n" + message)
            st.stop()
        load_csv.clear()                       # the scored file on disk has changed
        source_label = "Fresh run of the C++ detector (demo data)"
    elif uploaded is not None:
        df = load_csv(uploaded)
        source_label = uploaded.name
        if engine.needs_scoring(df):           # a raw file: let the C++ backend score it
            with st.spinner("Scoring the uploaded file with the C++ detector..."):
                ok, message, df = score_cached(df)
            if not ok:
                st.error("Could not score the file:\n\n" + message)
                st.stop()
            source_label += " (scored by the C++ detector)"
    elif default_file.exists():
        df = load_csv(default_file)
        source_label = "Project scored_transactions.csv"
    else:
        st.info("No data yet. Click **Run the C++ detector on demo data** in the sidebar, "
                "or upload a transactions CSV.")
        st.stop()
except Exception as e:
    st.error(f"Could not read the data: {e}")
    st.stop()

df = df.copy()
df.columns = [str(c).strip().lower() for c in df.columns]
# The C++ detector writes tx_id / account_id / receiver_id / risk_score / risk_level; accept both naming styles.
df = df.rename(columns={"tx_id": "id", "account_id": "sender", "receiver_id": "receiver",
                        "risk_score": "risk", "risk_level": "level"})
required = {"id", "sender", "receiver", "amount", "risk", "level"}
missing = required - set(df.columns)
if missing:
    st.error("Missing required columns: " + ", ".join(sorted(missing)))
    st.stop()

for col in ["amount", "risk"]:
    df[col] = pd.to_numeric(df[col], errors="coerce").fillna(0)
df["level"] = df["level"].fillna("NONE").astype(str).str.strip().str.upper()
if "reasons" not in df.columns:
    df["reasons"] = ""
df["reasons"] = df["reasons"].fillna("").astype(str)
df["reason_list"] = df["reasons"].apply(split_reasons)
df["reasons_text"] = df["reason_list"].apply(lambda r: ", ".join(pretty(x) for x in r))

if "timestamp" in df.columns:
    ts = pd.to_numeric(df["timestamp"], errors="coerce")
    if ts.notna().all():                       # epoch seconds
        df["time"] = pd.to_datetime(ts, unit="s", errors="coerce")
    else:                                      # "YYYY-MM-DD HH:MM:SS" text written by the C++ detector
        df["time"] = pd.to_datetime(df["timestamp"], errors="coerce")
else:
    df["time"] = pd.NaT

alerts = df[df["level"].isin(ALERT_LEVELS)]
high_risk = df[df["level"].isin(["HIGH", "CRITICAL"])]
avg_risk = df["risk"].mean() if len(df) else 0

# ---------------------------------------------------------------- KPIs
st.caption(f"Data source: {source_label}  ·  {len(df):,} transactions loaded")
m1, m2, m3, m4 = st.columns(4)
m1.metric("Total transactions", f"{len(df):,}")
m2.metric("Alerts detected", f"{len(alerts):,}", help="Medium, High and Critical")
m3.metric("High / critical risk", f"{len(high_risk):,}")
m4.metric("Average risk score", f"{avg_risk:.2f}")

# ---------------------------------------------------------------- charts
st.markdown("### Detection overview")
left, right = st.columns([1, 1])

with left:
    counts = (df["level"].value_counts().reindex(LEVEL_ORDER, fill_value=0)
              .rename_axis("Risk level").reset_index(name="Transactions"))
    fig = px.bar(counts, x="Risk level", y="Transactions", color="Risk level",
                 color_discrete_map=LEVEL_COLORS, text="Transactions",
                 title="Transactions by risk level")
    fig.update_traces(textposition="outside")
    fig.update_layout(showlegend=False)
    st.plotly_chart(style_fig(fig), width="stretch", theme=None)

with right:
    flagged = df[df["level"] != "NONE"]
    if len(flagged):
        exploded = flagged["reason_list"].explode().dropna()
        reason_counts = (exploded.map(pretty).value_counts().head(10)
                         .rename_axis("Detection reason").reset_index(name="Count"))
    else:
        reason_counts = pd.DataFrame({"Detection reason": ["No flagged transactions"], "Count": [0]})
    fig2 = px.bar(reason_counts, x="Count", y="Detection reason", orientation="h",
                  color_discrete_sequence=["#2876b8"], text="Count",
                  title="Most common detection reasons (flagged transactions)")
    fig2.update_layout(yaxis={"categoryorder": "total ascending"})
    st.plotly_chart(style_fig(fig2), width="stretch", theme=None)

# ---------------------------------------------------------------- explorer
st.markdown("### Transaction explorer")
c1, c2, c3 = st.columns([1.3, 1, 1])
with c1:
    search = st.text_input("Search transaction / account", placeholder="e.g. TX000123 or ACC045")
with c2:
    present = [l for l in LEVEL_ORDER if l in set(df["level"])]
    extra = sorted(set(df["level"]) - set(LEVEL_ORDER))
    chosen_level = st.selectbox("Risk level", ["All"] + present + extra)
with c3:
    min_risk = st.slider("Minimum risk score", 0.0, 1.0, 0.0, 0.05)

filtered = df
if search:
    s = search.strip()
    mask = (filtered["id"].astype(str).str.contains(s, case=False, na=False, regex=False)
            | filtered["sender"].astype(str).str.contains(s, case=False, na=False, regex=False)
            | filtered["receiver"].astype(str).str.contains(s, case=False, na=False, regex=False))
    filtered = filtered[mask]
if chosen_level != "All":
    filtered = filtered[filtered["level"] == chosen_level]
filtered = filtered[filtered["risk"] >= min_risk].sort_values("risk", ascending=False)

st.caption(f"Showing {len(filtered):,} of {len(df):,} transactions")
table = filtered[["id", "sender", "receiver", "amount", "time", "risk", "level", "reasons_text"]].rename(
    columns={"id": "ID", "sender": "Sender", "receiver": "Receiver", "amount": "Amount (₹)",
             "time": "Time (UTC)", "risk": "Risk", "level": "Level", "reasons_text": "Reasons"})
if df["time"].isna().all():
    table = table.drop(columns=["Time (UTC)"])
st.dataframe(
    table, width="stretch", hide_index=True, height=380,
    column_config={
        "Amount (₹)": st.column_config.NumberColumn(format="%.2f"),
        "Risk": st.column_config.ProgressColumn(min_value=0.0, max_value=1.0, format="%.2f"),
        "Time (UTC)": st.column_config.DatetimeColumn(format="DD MMM YYYY, HH:mm"),
    },
)

# ---------------------------------------------------------------- alert details
st.markdown("### Alert details")
if len(alerts):
    ranked = alerts.sort_values("risk", ascending=False)
    labels = {str(r.id): f"{r.id}  ·  {r.level}  ·  risk {r.risk:.2f}" for r in ranked.itertuples()}
    selected_id = st.selectbox("Select an alert to inspect", list(labels), format_func=labels.get,
                               help=f"{len(ranked):,} alerts, highest risk first")
    row = df[df["id"].astype(str) == selected_id].iloc[0]

    a, b, c = st.columns(3)
    a.metric("Transaction amount", f"₹{row['amount']:,.2f}")
    b.metric("Risk score", f"{row['risk']:.2f}")
    with c:
        color = LEVEL_COLORS.get(row["level"], "#9aa6b2")
        st.markdown('<div style="font-size:14px;color:#68758a;margin-top:6px">Risk level</div>'
                    f'<span class="badge" style="background:{color}">{row["level"]}</span>',
                    unsafe_allow_html=True)

    when = f"  ·  **Time:** {row['time']:%d %b %Y, %H:%M} UTC" if pd.notna(row["time"]) else ""
    st.markdown(f"**Transaction:** `{row['id']}`  ·  **Sender:** `{row['sender']}`  →  "
                f"**Receiver:** `{row['receiver']}`{when}")

    st.markdown("**Why it was flagged:**")
    if row["reason_list"]:
        st.markdown("".join(f'<span class="chip">{pretty(r)}</span>' for r in row["reason_list"]),
                    unsafe_allow_html=True)
    else:
        st.caption("No textual reason was recorded for this alert.")
else:
    st.info("No medium-or-higher risk alerts are present in the loaded file.")

st.markdown("---")
st.markdown('<p class="small-muted">BankGuard · Academic prototype · Risk scores are decision-support '
            'signals, not proof of fraud.</p>', unsafe_allow_html=True)
