CREATE OR REPLACE FUNCTION code._as_shipment(
    shipment          deploy.shipments,
    shipment_commands code.command_def[]
) RETURNS code.shipment AS $$
SELECT $1.shipment_id,
       to_json($2),
       $1.fqdns,
       $1.status::text,
       $1.parallel,
       $1.stop_on_error_count,
       $1.other_count,
       $1.done_count,
       $1.errors_count,
       $1.total_count,
       $1.created_at,
       $1.updated_at,
       $1.timeout,
       $1.tracing;
$$ LANGUAGE SQL IMMUTABLE;
