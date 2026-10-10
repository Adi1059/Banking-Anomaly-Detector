USE banking_anomaly;
-- 1. Ten most suspicious transactions
SELECT * FROM v_ranked_alerts LIMIT 10;
-- 2. Alerts per risk level
SELECT risk_level, COUNT(*) AS alerts FROM alerts GROUP BY risk_level ORDER BY FIELD(risk_level,'HIGH','MEDIUM','LOW');
-- 3. Accounts with the most alerts
SELECT t.account_id, COUNT(*) AS alerts, ROUND(SUM(t.amount),2) AS total_flagged
FROM alerts a JOIN transactions t ON t.tx_id = a.tx_id GROUP BY t.account_id ORDER BY alerts DESC LIMIT 10;
-- 4. Alerts raised by a given rule (reason text search)
SELECT tx_id, risk_score FROM alerts WHERE reasons LIKE '%structuring%' ORDER BY risk_score DESC;
-- 5. Hourly activity (odd-hour review)
SELECT HOUR(tx_time) AS hr, COUNT(*) AS tx_count FROM transactions GROUP BY hr ORDER BY hr;
