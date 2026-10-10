-- MySQL 8 schema for the Banking Anomaly Detector (3NF: accounts <- transactions <- alerts)
CREATE DATABASE IF NOT EXISTS banking_anomaly;
USE banking_anomaly;

CREATE TABLE IF NOT EXISTS accounts (
    account_id VARCHAR(16) PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS transactions (
    tx_id        VARCHAR(16) PRIMARY KEY,
    account_id   VARCHAR(16)   NOT NULL,
    receiver_id  VARCHAR(16)   NOT NULL,
    tx_time      DATETIME      NOT NULL,
    amount       DECIMAL(14,2) NOT NULL CHECK (amount >= 0),
    merchant     VARCHAR(32),
    is_injected  TINYINT(1)    NOT NULL DEFAULT 0,   -- ground-truth label (synthetic data only)
    anomaly_type VARCHAR(16),
    FOREIGN KEY (account_id)  REFERENCES accounts(account_id),
    FOREIGN KEY (receiver_id) REFERENCES accounts(account_id),
    INDEX idx_tx_account_time (account_id, tx_time),
    INDEX idx_tx_receiver (receiver_id)
);

CREATE TABLE IF NOT EXISTS alerts (
    alert_id    INT AUTO_INCREMENT PRIMARY KEY,
    tx_id       VARCHAR(16) NOT NULL,
    rule_score  DECIMAL(5,3) NOT NULL,
    graph_score DECIMAL(5,3) NOT NULL,
    ml_score    DECIMAL(5,3) NOT NULL,
    risk_score  DECIMAL(5,3) NOT NULL,
    risk_level  ENUM('NONE','LOW','MEDIUM','HIGH') NOT NULL,
    reasons     TEXT,
    FOREIGN KEY (tx_id) REFERENCES transactions(tx_id),
    INDEX idx_alert_risk (risk_score)
);

CREATE OR REPLACE VIEW v_ranked_alerts AS
SELECT a.alert_id, t.tx_id, t.account_id, t.receiver_id, t.tx_time, t.amount, a.risk_score, a.risk_level, a.reasons
FROM alerts a JOIN transactions t ON t.tx_id = a.tx_id
ORDER BY a.risk_score DESC;
