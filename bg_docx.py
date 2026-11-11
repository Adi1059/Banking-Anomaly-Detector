"""Read transactions from a Word file, make the input template, write the flagged report."""
import io

import pandas as pd
from docx import Document

from bg_rules import sample_df

COLS = ["transaction_id", "account_id", "receiver_id", "amount", "timestamp"]


def _bytes(doc):
    buf = io.BytesIO()
    doc.save(buf)
    return buf.getvalue()


def template_docx():
    doc = Document()
    doc.add_heading("BankGuard transaction input", 1)
    doc.add_paragraph("One row per transaction. Keep the header row. Amount in rupees. Timestamp as YYYY-MM-DD HH:MM. "
                      "Replace the sample rows with your own, then upload this file on the Check page.")
    table = doc.add_table(rows=1, cols=len(COLS))
    table.style = "Table Grid"
    for cell, name in zip(table.rows[0].cells, COLS):
        cell.text = name
    for row in sample_df().head(12).itertuples(index=False):
        cells = table.add_row().cells
        for cell, value in zip(cells, row):
            cell.text = str(value)
    return _bytes(doc)


def read_docx(data):
    doc = Document(io.BytesIO(data))
    frames = []
    for table in doc.tables:
        rows = [[c.text.strip() for c in r.cells] for r in table.rows]
        if len(rows) > 1:
            frames.append(pd.DataFrame(rows[1:], columns=[h.lower() for h in rows[0]]))
    if not frames:
        raise ValueError("No table found in that Word file. Use the template from this page.")
    return pd.concat(frames, ignore_index=True)


def report_docx(flagged, total, source):
    doc = Document()
    doc.add_heading("BankGuard flagged transactions", 1)
    doc.add_paragraph(f"Source: {source}. {total} transactions checked, {len(flagged)} flagged.")
    heads = ["ID", "From", "To", "Amount (INR)", "Risk", "Level", "Reasons"]
    table = doc.add_table(rows=1, cols=len(heads))
    table.style = "Table Grid"
    for cell, name in zip(table.rows[0].cells, heads):
        cell.text = name
    for r in flagged.itertuples():
        cells = table.add_row().cells
        for cell, value in zip(cells, [r.id, r.sender, r.receiver, f"{r.amount:,.2f}", f"{r.score:.2f}", r.level, r.reasons]):
            cell.text = str(value)
    doc.add_paragraph("Scored by the BankGuard quick check (rules and graph signals). Review flagged items manually.")
    return _bytes(doc)
