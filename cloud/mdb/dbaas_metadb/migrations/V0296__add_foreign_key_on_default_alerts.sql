ALTER TABLE 
	dbaas.alert 
ADD CONSTRAINT 
	fk_alert_default_alert FOREIGN KEY (metric_name) REFERENCES dbaas.default_alert(metric_name) 
ON DELETE RESTRICT;
