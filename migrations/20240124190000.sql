/**
 * Run a migration on this sql file with the default postgres user
 * ❯ sudo -u postgres psql -f 20240124190000.sql
 */

DO $$ 
BEGIN
  IF NOT EXISTS (SELECT FROM pg_user WHERE usename = 'andlifecare_app') THEN
    CREATE USER andlifecare_app;
  END IF;
END $$;

 -- `CREATE DATABASE` cannot be executed inside a transaction block, so we use `\gexec` instead.
SELECT 'CREATE DATABASE andlifecare OWNER andlifecare_app' WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'andlifecare')\gexec

\connect andlifecare;
CREATE SCHEMA IF NOT EXISTS app AUTHORIZATION andlifecare_app;

-- Let's make sure no one uses the public schema.
REVOKE ALL PRIVILEGES ON SCHEMA public FROM PUBLIC;

-- to use uuid_generate_v4() function
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

/**
 * After creating the andlifecare_app user, need to:
 * 
 * 1. ❯ sudo -u postgres psql
 * 2. postgres=# ALTER USER andlifecare_app WITH PASSWORD 'password';
 * 3. postgres=# \q
 * 4. ❯ sudo service postgresql restart
 * 5. ❯ psql -U andlifecare_app -h localhost -d andlifecare -W
 * 6. andlifecare=> ...
 * 
 * We don't parameterize the andlifecate_app user password in a script
 * because command-line arguments can be visible in process lists or logs
 */
