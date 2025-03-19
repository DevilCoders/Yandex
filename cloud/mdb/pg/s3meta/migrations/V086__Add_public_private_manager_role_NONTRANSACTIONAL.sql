ALTER TYPE s3.role_type ADD VALUE 'public_manager' AFTER 'presigner';
ALTER TYPE s3.role_type ADD VALUE 'private_manager' AFTER 'public_manager';
