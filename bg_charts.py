"""BankGuard charts."""
import pandas as pd
import plotly.graph_objects as go

from bg_theme import ALERT_LEVELS, GREEN, INK, LEVELS, LEVEL_FILL, TOMATO


def style_fig(fig, height):
    fig.update_layout(height=height, margin=dict(l=8, r=28, t=8, b=8), paper_bgcolor="#fff", plot_bgcolor="#fff",
                      showlegend=False, font=dict(family="Arial, Helvetica, sans-serif", color=INK, size=13))
    fig.update_xaxes(showgrid=True, gridcolor="#E3EBE7", zeroline=False, title=None)
    fig.update_yaxes(showgrid=False, title=None)
    return fig


def _hbar(labels, values, colors):
    fig = go.Figure(go.Bar(x=values, y=labels, orientation="h", text=values, textposition="outside",
                           marker=dict(color=colors), hovertemplate="%{y}: %{x}<extra></extra>"))
    fig.update_yaxes(autorange="reversed")
    return style_fig(fig, 320)


def risk_chart(data):
    lv = data["level"].value_counts().reindex(LEVELS[:-1], fill_value=0)
    return _hbar(list(lv.index), list(lv.values), [LEVEL_FILL[x] for x in lv.index])


def reason_chart(data):
    flagged = data.loc[data["level"] != "NONE", "reason_list"]
    rc = flagged.explode().replace("", pd.NA).dropna().value_counts().head(8)
    return _hbar(list(rc.index), list(rc.values), GREEN)


def account_chart(data):
    alerts = data[data["level"].isin(ALERT_LEVELS)]
    top = alerts["sender"].value_counts().head(8)
    return _hbar(list(top.index), list(top.values), INK)


def daily_chart(data):
    per_day = data[data["level"].isin(ALERT_LEVELS)].groupby(data["time"].dt.date).size()
    fig = go.Figure(go.Bar(x=per_day.index, y=per_day.values, marker=dict(color=TOMATO),
                           hovertemplate="%{x}: %{y} alerts<extra></extra>"))
    fig.update_yaxes(showgrid=True, gridcolor="#E3EBE7")
    return style_fig(fig, 240)
