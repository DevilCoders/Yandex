
CREATE TABLE deploy.shipment_commands (
    shipment_command_id  bigint NOT NULL GENERATED ALWAYS AS IDENTITY,
    shipment_id          bigint NOT NULL,
    type                 text   NOT NULL,
    arguments            text[],

    CONSTRAINT pk_shipment_commands PRIMARY KEY (shipment_command_id),
    CONSTRAINT check_type_length CHECK (
        char_length(type) BETWEEN 1 AND 512
    )
);


INSERT INTO deploy.shipment_commands
    (shipment_id, type)
SELECT
    shipment_id, type
  FROM deploy.shipments;

CREATE INDEX i_shipment_commands_shipment_id
    ON deploy.shipment_commands (shipment_id);

ALTER TABLE deploy.shipments DROP CONSTRAINT check_type_length;
ALTER TABLE deploy.shipments DROP COLUMN type;

ALTER TABLE deploy.commands
  ADD COLUMN shipment_command_id bigint;

UPDATE deploy.commands
   SET shipment_command_id = sc.shipment_command_id
  FROM deploy.shipment_commands sc
 WHERE sc.shipment_id = commands.shipment_id;

ALTER TABLE deploy.commands ALTER COLUMN shipment_command_id SET NOT NULL;

ALTER TABLE deploy.commands DROP CONSTRAINT fk_commands_shipment_id;
ALTER TABLE deploy.commands DROP CONSTRAINT check_type_length;
DROP INDEX deploy.i_commands_shipment_id;
ALTER TABLE deploy.commands DROP COLUMN type;
ALTER TABLE deploy.commands DROP COLUMN shipment_id;

CREATE INDEX i_commands_shipment_command_id
    ON deploy.commands (shipment_command_id);

ALTER TABLE deploy.commands
ADD CONSTRAINT fk_commands_shipment_command_id FOREIGN KEY (shipment_command_id)
        REFERENCES deploy.shipment_commands (shipment_command_id) ON DELETE CASCADE;
