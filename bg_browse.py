"""Searchable transaction explorer."""
import pandas as pd
import streamlit as st

def browse(data, total):
    st.markdown("### Transaction explorer")
    a, b, c = st.columns([1.2, 1, 1])
    query = a.text_input("Search transaction or account", placeholder="Transaction ID / account")
    levels = b.multiselect("Risk level", ["CRITICAL", "HIGH", "MEDIUM", "LOW", "NONE"],
                           default=["CRITICAL", "HIGH", "MEDIUM", "LOW", "NONE"])
min_score = c.slider(
    "Minimum risk score",
    0.0, 1.0, 0.0, 0.05,
    key="browse_min_risk_score"
)    view = data[data["level"].isin(levels) & (data["score"] >= min_score)].copy()
    if query.strip():
        term = query.strip().lower()
        mask = (
            view["id"].astype(str).str.lower().str.contains(term, na=False) |
            view["sender"].astype(str).str.lower().str.contains(term, na=False) |
            view["receiver"].astype(str).str.lower().str.contains(term, na=False)
        )
        view = view[mask]
    view = view.sort_values("score", ascending=False)
    st.caption(f"{len(view):,} matching transactions out of {total:,}.")
    display = view[["id", "sender", "receiver", "amount", "time", "score", "level", "reasons"]].copy()
    st.dataframe(display, use_container_width=True, hide_index=True,
                 column_config={
                     "amount": st.column_config.NumberColumn("Amount (INR)", format="₹%.2f"),
                     "score": st.column_config.ProgressColumn("Risk score", min_value=0, max_value=1, format="%.2f"),
                     "time": st.column_config.DatetimeColumn("Timestamp", format="DD MMM YYYY, HH:mm")
                 })
    st.download_button("Download filtered transactions (CSV)", view.to_csv(index=False).encode("utf-8"),
                       file_name="bankguard_transactions.csv", mime="text/csv")
