CREATE INDEX i_shipments_created_at_timeout
    ON deploy.shipments (created_at, timeout)
 WHERE STATUS = 'INPROGRESS'::deploy.shipment_status;
