"""Facts, incidents and prevention tips shown on the site. Checked 11 Nov 2026."""
BS = "https://www.business-standard.com/industry/banking/bank-fraud-cases-halve-but-value-climbs-to-48-000-crore-rbi-data-126052900631_1.html"
FPJ = "https://www.freepressjournal.in/business/bank-fraud-amount-jumps-46-to-48021-crore-in-fy26-rbi-weighs-kill-switch-to-curb-digital-risks"
IANS = "https://ianslive.in/indians-lose-over-rs-52976-crore-to-cyber-frauds-over-six-years-report--20260103154943"
DARR = "https://www.freepressjournal.in/tech/digital-arrest-fraud-over-21900-complaints-involving-647-crore-registered-in-2026-centre-tells-supreme-court"
UPS = "https://upstox.com/news/upstox-originals/investing/indian-banks-face-a-stiff-fight-against-rising-frauds/article-196369/"
CP = "https://cyberpeace.org/resources/blogs/inside-indias-cyber-fraud-economy-bank-insiders-crypto-and-mule-accounts"

FACTS = [
    dict(num="\u20b948,021 cr", title="Fraud reported by Indian banks in FY26",
         note="Across 10,114 cases. The value rose 46% from \u20b932,803 cr a year earlier, even though the case count fell.",
         src="RBI Annual Report 2025-26, via Business Standard", url=BS),
    dict(num="84.9%", title="of that value was loan fraud",
         note="Advances frauds came to \u20b940,774 cr across 8,640 cases.",
         src="RBI Annual Report 2025-26, via Free Press Journal", url=FPJ),
    dict(num="293", title="digital-payment frauds reported by banks",
         note="Down from 13,332 in FY25 and 28,836 in FY24 in the banks' own reporting.",
         src="RBI data, via Business Standard", url=BS),
    dict(num="\u20b919,813 cr", title="lost to cyber cheating in 2025",
         note="About 2.18 million complaints reached the national portal. Roughly 77% of the amount was investment scams.",
         src="I4C data, via IANS", url=IANS),
    dict(num="21,900+", title="digital arrest complaints, Jan to Aug 2026",
         note="Worth over \u20b9647 cr. 2024 had 123,672 complaints (\u20b91,935 cr) and 2025 had 58,239 (\u20b91,211 cr).",
         src="I4C status report, via Free Press Journal", url=DARR),
    dict(num="23 banks", title="used RBI-backed MuleHunter.AI by Dec 2025",
         note="An AI platform that spots mule accounts, the accounts used to move stolen money.",
         src="Upstox, citing RBI", url=UPS),
]

INCIDENTS = [
    dict(date="30 Oct 2026", title="Hyderabad police break a mule-account supply ring",
         body="Police arrested gang members accused of procuring bank accounts for fraudsters. They found 83 such accounts across 22 branches, 11 of them tied to 14 fraud transactions worth about \u20b910.04 crore.",
         signal="Graph: Fan in", src="Siasat", url="https://www.siasat.com/3-held-for-selling-mule-accounts-linked-to-rs-10-04-crore-fraud-3567539/"),
    dict(date="30 Oct 2026", title="ED traces \u20b9400 crore laundered through mule accounts and crypto",
         body="The Enforcement Directorate's Chennai office arrested two people under the PMLA. The case grew out of a Cyberabad FIR, and the money moved through mule accounts and cryptocurrency channels.",
         signal="Graph: Linked cluster", src="Siasat", url="https://www.siasat.com/fir-in-cyberabad-helps-ed-uncover-rs-400-crore-cyber-fraud-3571093/"),
    dict(date="23 Oct 2026", title="Delhi Police arrest five bankers in a fraud syndicate",
         body="Six people were arrested, five of them bankers, in an international syndicate. The case shows that the risk can sit inside the bank as well as outside it.",
         signal="Large transfer to new payee", src="CyberPeace", url=CP),
    dict(date="22 Sep 2026", title="Bengaluru probe finds 507 suspected mule accounts",
         body="Three men were arrested in an online trading and investment scam. Investigators found 507 suspected mule accounts and possible international links.",
         signal="Graph: Fan in", src="Sumsub", url="https://sumsub.com/media/news/bengaluru-police-uncover-507-mule-accounts-investment-fraud/"),
    dict(date="Sep 2026", title="\u20b912.84 crore taken from a corporate account",
         body="Five people were arrested over money taken from Hero FinCorp's account, including a private-bank relationship manager. Part of it was converted into the USDT stablecoin.",
         signal="Large transfer to new payee", src="CyberPeace", url=CP),
]

TIPS_CUSTOMER = [
    "Treat any \u201cdigital arrest\u201d call as fraud. No agency arrests people over a video call or asks for money to close a case. Hang up and call 1930.",
    "Never share an OTP, PIN or CVV, even with someone who sounds like the bank.",
    "Be wary of guaranteed returns. About 77% of the money lost in 2025 went to investment scams.",
    "Do not lend or rent out your account. Accounts used to pass on stolen money are called mule accounts.",
    "Check the payee name and UPI ID before you approve a request or a collect message.",
    "Report fast: call 1930 or file at cybercrime.gov.in, and tell your bank straight away.",
]
TIPS_BANK = [
    "Watch each account for sudden spikes against its own history, not just a fixed limit.",
    "Use graph checks for fan-in, fan-out and clusters of linked accounts.",
    "Add friction to large first-time payee transfers. RBI is looking at delayed or lagged credit.",
    "Screen for mule accounts at opening and keep screening afterwards.",
    "Control insider access to alert and freeze lists, and log who looks at them.",
    "Feed police and portal complaints back into the detector so it keeps learning.",
]

PIPELINE = [
    ("1", "Rules", "Plain checks on every transaction.", "Amount spike, Large transfer to new payee, Structuring, Repeated transaction"),
    ("2", "Graph", "Looks at who pays whom.", "Fan in, Fan out, Linked clusters of accounts"),
    ("3", "Machine learning", "An Isolation Forest scores how unusual a transaction is.", "ML outlier"),
    ("4", "Hybrid score", "Signals combine into one 0 to 1 score and a level.", "None, Low, Medium, High, Critical"),
]

SOURCES = [(f["src"], f["url"]) for f in FACTS] + [(i["src"], i["url"]) for i in INCIDENTS]
