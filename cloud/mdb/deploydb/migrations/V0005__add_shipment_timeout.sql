ALTER TABLE deploy.shipments
  ADD COLUMN timeout interval;


UPDATE deploy.shipments
   SET timeout = INTERVAL '10 minutes';

ALTER TABLE deploy.shipments ALTER COLUMN timeout SET NOT NULL;
