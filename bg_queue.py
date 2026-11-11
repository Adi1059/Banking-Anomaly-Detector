"""BankGuard review queue: ranked alerts with a case panel."""
import html
import inspect

import pandas as pd
import streamlit as st

from bg_theme import ALERT_LEVELS, INK, LEVEL_FILL, LEVEL_TEXT, MIST, paint

STRETCH = {"width": "stretch"} if "width" in inspect.signature(st.button).parameters else {"use_container_width": True}


def case_panel(row, data):
    fill, ink = LEVEL_FILL.get(row["level"], MIST), LEVEL_TEXT.get(row["level"], INK)
    when = row["time"].strftime("%d %b %Y, %H:%M") if pd.notna(row["time"]) else "unknown"
    chips = "".join(f'<span class="chip">{html.escape(r)}</span>' for r in row["reason_list"]) or "None recorded"
    pct = int(round(min(max(row["score"], 0), 1) * 100))
    st.markdown(
        f'<div class="case" style="border-left-color:{fill}"><div class="case-head">'
        f'<span class="case-id">{html.escape(row["id"])}</span>'
        f'<span class="pill" style="background:{fill};color:{ink}">{row["level"]}</span></div>'
        f'<div class="score"><b>{row["score"]:.2f}</b><div class="meter"><i style="width:{pct}%;background:{fill}"></i></div></div>'
        f'<div class="kv"><div><small>From</small>{html.escape(row["sender"])}</div>'
        f'<div><small>To</small>{html.escape(row["receiver"] or "unknown")}</div>'
        f'<div><small>Amount</small>\u20b9{row["amount"]:,.2f}</div><div><small>When</small>{when}</div></div>'
        f'<div class="note">Why it was flagged</div>{chips}</div>', unsafe_allow_html=True)
    same = data[data["sender"] == row["sender"]]
    n_alerts = int(same["level"].isin(ALERT_LEVELS).sum())
    st.markdown(f'<div class="h">Other activity from {html.escape(row["sender"])}</div>'
                f'<div class="note">{len(same)} transactions, {n_alerts} of them alerts</div>', unsafe_allow_html=True)
    show = same.sort_values("score", ascending=False).head(8)[["id", "receiver", "amount", "score", "level"]]
    st.dataframe(show.style.map(paint, subset=["level"]) if hasattr(show.style, "map") else show,
                 hide_index=True, **STRETCH, column_config={
                     "id": "ID", "receiver": "To", "amount": st.column_config.NumberColumn("Amount (\u20b9)", format="%.2f"),
                     "score": st.column_config.ProgressColumn("Risk", min_value=0, max_value=1, format="%.2f"),
                     "level": "Level"})


def queue(data):
    left, right = st.columns([1, 1.15], gap="large")
    with left:
        st.markdown('<div class="h">Review queue</div>', unsafe_allow_html=True)
        pick = st.radio("Show", ["All alerts", "High and critical"], horizontal=True, label_visibility="collapsed")
        levels = ALERT_LEVELS if pick == "All alerts" else ["CRITICAL", "HIGH"]
        q = data[data["level"].isin(levels)].sort_values("score", ascending=False).head(300).reset_index(drop=True)
        if q.empty:
            st.info("No alerts at this level.")
            return
        st.markdown(f'<div class="note">{len(q)} shown, highest risk first. Click a row to open the case.</div>',
                    unsafe_allow_html=True)
        view = q[["id", "sender", "amount", "score", "level"]]
        styler = view.style.map(paint, subset=["level"]) if hasattr(view.style, "map") else view.style.applymap(paint, subset=["level"])
        event = st.dataframe(styler, hide_index=True, **STRETCH, height=520, on_select="rerun",
                             selection_mode="single-row", key="queue_sel", column_config={
                                 "id": "ID", "sender": "From",
                                 "amount": st.column_config.NumberColumn("Amount (\u20b9)", format="%.2f"),
                                 "score": st.column_config.ProgressColumn("Risk", min_value=0, max_value=1, format="%.2f"),
                                 "level": st.column_config.TextColumn("Level", width="small")})
    rows = event.selection.rows if event is not None else []
    idx = rows[0] if rows and rows[0] < len(q) else 0
    with right:
        case_panel(q.iloc[idx], data)
