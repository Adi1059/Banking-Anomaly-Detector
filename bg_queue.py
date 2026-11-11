"""Analyst review queue with filters and downloadable report."""
import pandas as pd
import streamlit as st
from bg_theme import paint
from bg_docx import report_docx

def queue(data, source="Uploaded data"):
    st.markdown("### Review queue")
    st.caption("Prioritise the highest-risk transactions first. A flag is a signal for review, not proof of fraud.")
    alerts = data[data["level"].isin(["CRITICAL", "HIGH", "MEDIUM"])].copy()
    if alerts.empty:
        st.success("No medium-or-higher risk transactions in this dataset.")
        return
    c1, c2, c3 = st.columns([1.2, 1, 1])
    levels = c1.multiselect("Risk level", ["CRITICAL", "HIGH", "MEDIUM"], default=["CRITICAL", "HIGH", "MEDIUM"])
    min_score = c2.slider("Minimum risk score", 0.0, 1.0, 0.0, 0.05)
    search = c3.text_input("Search account or transaction", placeholder="Type an ID...")
    filtered = alerts[alerts["level"].isin(levels) & (alerts["score"] >= min_score)]
    if search.strip():
        term = search.strip().lower()
        filtered = filtered[
            filtered["id"].astype(str).str.lower().str.contains(term, na=False) |
            filtered["sender"].astype(str).str.lower().str.contains(term, na=False) |
            filtered["receiver"].astype(str).str.lower().str.contains(term, na=False)
        ]
    filtered = filtered.sort_values(["score", "amount"], ascending=False)
    st.caption(f"Showing {len(filtered):,} of {len(alerts):,} flagged transactions.")
    view = filtered[["id", "sender", "receiver", "amount", "time", "score", "level", "reasons"]].copy()
    view["amount"] = view["amount"].map(lambda x: f"₹{x:,.2f}")
    view["score"] = view["score"].map(lambda x: f"{x:.2f}")
    st.dataframe(view, use_container_width=True, hide_index=True)
    st.download_button("Download filtered alerts (CSV)", filtered.to_csv(index=False).encode("utf-8"),
                       file_name="bankguard_alerts.csv", mime="text/csv")
    st.download_button("Download flagged report (Word)", report_docx(filtered, len(data), source),
                       file_name="bankguard_flagged_report.docx",
                       mime="application/vnd.openxmlformats-officedocument.wordprocessingml.document")
