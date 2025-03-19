ALTER TYPE s3.role_type ADD VALUE 'owner' AFTER 'admin';
ALTER TYPE s3.role_type ADD VALUE 'reader' AFTER 'owner';
