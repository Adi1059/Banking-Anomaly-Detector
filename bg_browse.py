"""BankGuard all-transactions browser."""
import inspect

import streamlit as st

from bg_theme import LEVELS, paint

STRETCH = {"width": "stretch"} if "width" in inspect.signature(st.button).parameters else {"use_container_width": True}


def browse(data, total):
    f1, f2, f3 = st.columns([1.3, 1.3, 1])
    query = f1.text_input("Search by transaction or account", placeholder="TX002816 or ACC0048")
    chosen = f2.multiselect("Risk level", LEVELS, placeholder="All levels")
    min_score = f3.slider("Minimum risk score", 0.0, 1.0, 0.0, 0.01)

    view = data[data["score"] >= min_score]
    if chosen:
        view = view[view["level"].isin(chosen)]
    if query.strip():
        q = query.strip()
        hit = view[["id", "sender", "receiver"]].apply(lambda s: s.str.contains(q, case=False, regex=False))
        view = view[hit.any(axis=1)]
    view = view.sort_values("score", ascending=False)

    m1, m2 = st.columns([4, 1])
    m1.markdown(f'<div class="note">Showing {len(view):,} of {total:,} transactions</div>', unsafe_allow_html=True)
    m2.download_button("Download CSV", view.drop(columns="reason_list").to_csv(index=False).encode(),
                       "bankguard_filtered.csv", "text/csv", **STRETCH)
    table = view[["id", "sender", "receiver", "amount", "time", "score", "level", "reasons"]]
    styler = table.style.map(paint, subset=["level"]) if hasattr(table.style, "map") else table.style.applymap(paint, subset=["level"])
    st.dataframe(styler, hide_index=True, **STRETCH, height=520, column_config={
        "id": "ID", "sender": "Sender", "receiver": "Receiver",
        "amount": st.column_config.NumberColumn("Amount (\u20b9)", format="%.2f"),
        "time": st.column_config.DatetimeColumn("Time", format="DD MMM YYYY, HH:mm"),
        "score": st.column_config.ProgressColumn("Risk", min_value=0, max_value=1, format="%.2f"),
        "level": st.column_config.TextColumn("Level", width="small"),
        "reasons": st.column_config.TextColumn("Reasons", width="large")})
