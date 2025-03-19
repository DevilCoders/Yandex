ALTER TYPE cms.instance_operation_status ADD VALUE 'ok-pending' BEFORE 'ok';
ALTER TYPE cms.instance_operation_status ADD VALUE 'reject-pending' BEFORE 'rejected';
