DELETE FROM billing.tracks
WHERE cluster_id = %(cluster_id)s
  AND bill_type  = %(bill_type)s
