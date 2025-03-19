CREATE OR REPLACE FUNCTION v1_impl.lock_object(
    i_bid uuid,
    i_name text
) RETURNS void
LANGUAGE plpgsql AS $$
BEGIN
    /* NOTE: records in s3.object_locks should present only during active transaction
       and before locking function exits should be removed.
       But if id case of error records remains in table it should not affect locking process
       and we use ON CONFLICT for this

       This INSERT locks row FOR UPDATE, no need to SELECT FOR UPDATE.
    */
    INSERT INTO s3.object_locks (bid, name)
    VALUES (i_bid, i_name)
    ON CONFLICT (bid, name) DO UPDATE SET name=i_name;

    /* DELETE instantly to avoid garbage in s3.object_locks.
       DELETE does not prevent row lock.
    */
    DELETE FROM s3.object_locks
    WHERE bid=i_bid AND name=i_name;
END;
$$;
