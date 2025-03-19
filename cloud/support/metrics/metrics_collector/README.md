# Metrics Collector  

later


## Database install  

```bash
mysql -uroot -p
```

```sql
CREATE DATABASE cloudsupport DEFAULT CHARACTER SET utf8;
DEFAULT COLLATE utf8_general_ci;
GRANT ALL PRIVILEGES ON cloudsupport.* TO 'collector' IDENTIFIED BY 'password';
FLUSH PRIVILEGES;
\q
```
  
**Install tables to database**

```bash
mysql cloudsupport -umetrics_collector -p < install.sql
```
