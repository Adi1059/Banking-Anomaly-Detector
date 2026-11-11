"""BankGuard theme: emerald case-desk look."""
import streamlit as st

GREEN, DEEP, PAPER, INK = "#0A5C3E", "#06281C", "#F3F6F4", "#06281C"
TOMATO, AMBER, SKY, MIST, CRIMSON = "#E5322D", "#F2A900", "#3FA7D6", "#B7C4BD", "#8E0F2B"
LEVELS = ["CRITICAL", "HIGH", "MEDIUM", "LOW", "NONE"]
LEVEL_FILL = {"CRITICAL": CRIMSON, "HIGH": TOMATO, "MEDIUM": AMBER, "LOW": SKY, "NONE": MIST}
LEVEL_TEXT = {"CRITICAL": "#fff", "HIGH": "#fff", "MEDIUM": INK, "LOW": INK, "NONE": INK}
ALERT_LEVELS = ["CRITICAL", "HIGH", "MEDIUM"]

CSS = """
<style>
html, body, .stApp, [class*="st-"] { font-family: Arial, Helvetica, sans-serif; }
.stApp { background: %PAPER%; }
header[data-testid="stHeader"] { background: transparent; }
.block-container { padding-top: 1.5rem; max-width: 1300px; }
[data-testid="stSidebar"] { background: #fff; border-right: 2px solid %GREEN%; }
[data-testid="stSidebar"] h3 { color: %GREEN%; font-weight: 800; }
[data-testid="stSidebar"] .stButton button { background: %GREEN%; border: 0; border-radius: 6px; }
[data-testid="stSidebar"] .stButton button * { color: #fff !important; font-weight: 700; }
.band { display: flex; justify-content: space-between; align-items: flex-end; gap: 2rem; background: %GREEN%; color: #fff;
        padding: 1.6rem 2rem 1.3rem; border-radius: 10px 10px 0 0; border-bottom: 6px solid %TOMATO%; }
.brand { font-size: 2.8rem; font-weight: 800; letter-spacing: -.03em; line-height: 1; }
.tag { max-width: 26rem; text-align: right; color: #cfe8da; line-height: 1.4; }
.strip { display: grid; grid-template-columns: repeat(4, 1fr); background: %DEEP%; color: #cfe8da; border-radius: 0 0 10px 10px; margin-bottom: 1.4rem; }
.strip div { padding: 1rem 1.6rem; border-right: 1px solid #1c4a38; font-size: .9rem; }
.strip div:last-child { border-right: 0; }
.strip span { display: block; font-size: 2.4rem; font-weight: 800; color: #fff; line-height: 1.1; }
.strip .hot span { color: %AMBER%; }
@media (max-width: 900px) { .strip { grid-template-columns: repeat(2, 1fr); } .band { flex-direction: column; align-items: flex-start; } .tag { text-align: left; } }
[data-baseweb="tab-list"] { gap: 1.6rem; border-bottom: 2px solid #cfdcd5; }
button[data-baseweb="tab"] { font-weight: 700; font-size: 1.05rem; }
.h { font-size: 1.15rem; font-weight: 800; color: %GREEN%; margin: .6rem 0 .4rem; }
.note { color: %INK%; opacity: .7; font-size: .9rem; margin-bottom: .6rem; }
[data-testid="stPlotlyChart"], [data-testid="stDataFrame"] { background: #fff; border: 1px solid #cfdcd5; border-radius: 8px; padding: .3rem; overflow: hidden; }
[data-testid="stMain"] div[data-baseweb="input"], [data-testid="stMain"] div[data-baseweb="select"] > div
  { background: #fff !important; border: 1px solid #9db3a8 !important; border-radius: 6px !important; }
[data-testid="stMain"] input, [data-testid="stMain"] div[data-baseweb="select"] * { color: %INK% !important; }
[data-testid="stMain"] .stDownloadButton button { background: %GREEN%; border: 0; border-radius: 6px; }
[data-testid="stMain"] .stDownloadButton button * { color: #fff !important; font-weight: 700; }
.case { background: #fff; border: 1px solid #cfdcd5; border-left: 8px solid %GREEN%; border-radius: 8px; padding: 1.2rem 1.4rem; }
.case-head { display: flex; align-items: center; gap: .8rem; }
.case-id { font-size: 1.7rem; font-weight: 800; }
.pill { font-weight: 700; padding: .15rem .8rem; border-radius: 4px; font-size: .85rem; }
.score { display: flex; align-items: center; gap: 1rem; margin: 1rem 0; }
.score b { font-size: 3rem; line-height: 1; }
.meter { flex: 1; height: 14px; background: #e3ebe7; border-radius: 3px; overflow: hidden; }
.meter i { display: block; height: 100%; }
.kv { display: grid; grid-template-columns: 1fr 1fr; gap: .7rem 1.5rem; margin-bottom: 1rem; }
.kv div { font-size: 1.05rem; } .kv small { display: block; opacity: .6; font-size: .8rem; }
.chip { display: inline-block; background: #DCEFE4; color: %GREEN%; font-weight: 700; border-radius: 4px; padding: .2rem .65rem; margin: 0 .4rem .4rem 0; }
</style>
"""


def inject():
    css = CSS
    for k, v in {"%PAPER%": PAPER, "%GREEN%": GREEN, "%DEEP%": DEEP, "%INK%": INK, "%TOMATO%": TOMATO, "%AMBER%": AMBER}.items():
        css = css.replace(k, v)
    st.markdown(css, unsafe_allow_html=True)


def paint(v):
    return f"background-color:{LEVEL_FILL.get(v, MIST)};color:{LEVEL_TEXT.get(v, INK)};font-weight:700;text-align:center"
