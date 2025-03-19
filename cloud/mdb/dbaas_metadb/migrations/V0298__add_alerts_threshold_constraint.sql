
ALTER TABLE 
	dbaas.alert
ADD CONSTRAINT 
	alert_has_thresholds 
CHECK (critical_threshold IS NOT NULL or warning_threshold IS NOT NULL);

