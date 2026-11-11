"""Plotly charts used by the BankGuard dashboard."""
import plotly.express as px
import plotly.graph_objects as go

LEVEL_ORDER = ["CRITICAL", "HIGH", "MEDIUM", "LOW", "NONE"]
LEVEL_COLORS = {"CRITICAL": "#8b1e3f", "HIGH": "#e05252", "MEDIUM": "#e7a23b",
                "LOW": "#4c83c3", "NONE": "#9aa6b2"}

def _base(fig):
    fig.update_layout(
        template="plotly_white", paper_bgcolor="white", plot_bgcolor="white",
        font=dict(color="#1b2740"), margin=dict(l=12, r=12, t=48, b=12),
        title_font=dict(size=16, color="#1b2740"), legend_title_text=""
    )
    fig.update_xaxes(gridcolor="#edf0f5", zerolinecolor="#edf0f5")
    fig.update_yaxes(gridcolor="#edf0f5", zerolinecolor="#edf0f5")
    return fig

def risk_chart(data):
    counts = data["level"].value_counts().reindex(LEVEL_ORDER, fill_value=0).rename_axis("Risk level").reset_index(name="Transactions")
    fig = px.bar(counts, x="Risk level", y="Transactions", color="Risk level",
                 category_orders={"Risk level": LEVEL_ORDER}, color_discrete_map=LEVEL_COLORS,
                 title="Transactions by risk level", text="Transactions")
    fig.update_traces(textposition="outside")
    return _base(fig)

def reason_chart(data):
    exploded = data[["reason_list"]].explode("reason_list").dropna()
    if exploded.empty:
        fig = go.Figure()
        fig.add_annotation(text="No detection reasons were included in this file.", showarrow=False)
        fig.update_layout(title="Why transactions were flagged")
        return _base(fig)
    counts = exploded["reason_list"].value_counts().head(10).sort_values().reset_index()
    counts.columns = ["Reason", "Transactions"]
    fig = px.bar(counts, x="Transactions", y="Reason", orientation="h",
                 title="Most common detection reasons", text="Transactions")
    fig.update_traces(marker_color="#5b2ee0", textposition="outside")
    return _base(fig)

def account_chart(data):
    alerts = data[data["level"].isin(["CRITICAL", "HIGH", "MEDIUM"])]
    counts = alerts.groupby("sender").size().sort_values(ascending=False).head(10).sort_values().reset_index(name="Alerts")
    if counts.empty:
        fig = go.Figure()
        fig.add_annotation(text="No alerts to group by sender account.", showarrow=False)
        fig.update_layout(title="Accounts with the most alerts")
        return _base(fig)
    fig = px.bar(counts, x="Alerts", y="sender", orientation="h", title="Accounts with the most alerts")
    fig.update_traces(marker_color="#e05252")
    return _base(fig)

def daily_chart(data):
    valid = data.dropna(subset=["time"]).copy()
    if valid.empty:
        fig = go.Figure()
        fig.add_annotation(text="No usable timestamps in this file.", showarrow=False)
        return _base(fig)
    valid["day"] = valid["time"].dt.date.astype(str)
    alerts = valid[valid["level"].isin(["CRITICAL", "HIGH", "MEDIUM"])]
    counts = alerts.groupby("day").size().reset_index(name="Alerts")
    fig = px.line(counts, x="day", y="Alerts", markers=True, title="Alerts over time")
    return _base(fig)
