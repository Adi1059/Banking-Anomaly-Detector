"""BankGuard dashboard styling."""
import streamlit as st

ALERT_LEVELS = ["CRITICAL", "HIGH", "MEDIUM"]
LEVEL_COLORS = {"CRITICAL": "#8b1e3f", "HIGH": "#e05252", "MEDIUM": "#e7a23b",
                "LOW": "#4c83c3", "NONE": "#9aa6b2"}

CSS = """
<style>
.stApp { background:#f5f7fb; color:#1b2740; }
.block-container { max-width:1450px; padding-top:2rem; padding-bottom:3rem; }
header[data-testid="stHeader"] { background:#f5f7fb; }
section[data-testid="stSidebar"] { background:#fff; border-right:1px solid #e7ebf2; }
section[data-testid="stSidebar"] * { color:#1b2740; }
.hero { background:linear-gradient(110deg,#101d3a,#183c68); padding:28px 32px;
        border-radius:18px; margin-bottom:22px; }
.hero h1 { color:#fff !important; font-size:2.1rem; margin:0; }
.hero p { color:#d8e5f5 !important; margin:.5rem 0 0; }
.eyebrow { color:#a9c5e8 !important; font-size:.75rem; letter-spacing:.12em; text-transform:uppercase; }
div[data-testid="stMetric"] { background:#fff; padding:15px 18px; border:1px solid #e7ebf2; border-radius:12px; }
div[data-testid="stMetric"] label, div[data-testid="stMetric"] [data-testid="stMetricValue"] { color:#1b2740 !important; }
.stTabs [data-baseweb="tab"] { font-weight:600; }
.stButton button, .stDownloadButton button { border-radius:9px; font-weight:600; }
[data-testid="stDataFrame"], [data-testid="stPlotlyChart"] { background:#fff; border-radius:12px; }
.small-muted { color:#68758a; font-size:.85rem; }
</style>
"""

def inject():
    st.markdown(CSS, unsafe_allow_html=True)

def paint(level):
    color = LEVEL_COLORS.get(str(level).upper(), "#9aa6b2")
    return f"background-color:{color};color:white;font-weight:700;text-align:center"
