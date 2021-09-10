CREATE TABLE constants (
    _ bool PRIMARY KEY DEFAULT true,
    version int
);

INSERT INTO constants (version) VALUES (1);

CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS citext;
DROP DOMAIN IF EXISTS email;
CREATE DOMAIN email AS citext
  CHECK ( value ~ '^[a-zA-Z0-9.!#$%&''*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$' );

CREATE TABLE permissions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name TEXT UNIQUE
);

INSERT INTO permissions (name) VALUES
    ('list-permissions'),
    ('edit-permissions'),
    ('list-users'),
    ('edit-users');
    
CREATE TABLE roles (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name TEXT UNIQUE
);

INSERT INTO roles (name) VALUES ('admin');

CREATE TABLE roles_permissions (
    role_id UUID NOT NULL REFERENCES roles(id) ON DELETE CASCADE ON UPDATE CASCADE,
    permission_id UUID NOT NULL REFERENCES permissions(id) ON DELETE CASCADE ON UPDATE CASCADE
);

INSERT INTO roles_permissions (role_id, permission_id) VALUES (
    (SELECT id FROM roles WHERE name = 'admin'),
    (SELECT id FROM permissions WHERE name = 'list-permissions')
), (
    (SELECT id FROM roles WHERE name = 'admin'),
    (SELECT id FROM permissions WHERE name = 'edit-permissions')
), (
    (SELECT id FROM roles WHERE name = 'admin'),
    (SELECT id FROM permissions WHERE name = 'list-users')
), (
    (SELECT id FROM roles WHERE name = 'admin'),
    (SELECT id FROM permissions WHERE name = 'edit-users')
);

CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    email VARCHAR(31) UNIQUE,
    first_name VARCHAR(16),
    last_name VARCHAR(16)
);

INSERT INTO users (email, first_name, last_name) 
VALUES ('admin@caltech.edu', 'admin', 'admin');

CREATE TABLE passwords (
    user_id UUID PRIMARY KEY NOT NULL REFERENCES users(id) ON DELETE CASCADE ON UPDATE CASCADE,
    salt BYTEA,
    hash BYTEA
);

INSERT INTO passwords (user_id, salt, hash) VALUES (
    (SELECT id FROM users WHERE email = 'admin@caltech.edu'),
    sha256(''),
    sha256('admin' || sha256(''))
);

CREATE TABLE users_roles (
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE ON UPDATE CASCADE,
    role_id UUID NOT NULL REFERENCES roles(id) ON DELETE CASCADE ON UPDATE CASCADE
);

INSERT INTO users_roles (user_id, role_id) VALUES (
    (SELECT id FROM users WHERE email = 'admin@caltech.edu'),
    (SELECT id FROM roles WHERE name = 'admin')
);
