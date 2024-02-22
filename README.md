# andlifecare

An online healthcare platform for managing communication and paperwork between doctors and patients.

## Tech

This project uses `C89`, which provides precise control over hardware resources and language simplicity.

## Architecture

### Consumer layer - `web` & `api` directories

Responsible for rendering and sending responses to the client.

### Domain layer - `core` directory

Handles data validation, as well as the persistence or retrieval of data from the database and 3rd party services.

## Tooling

| Library  | Description                           |
|----------|---------------------------------------|
| [PostgreSQL 16](https://www.postgresql.org/docs/16/index.html)  | RDMS (relational database management system) |
| [Argon2](https://github.com/p-h-c/phc-winner-argon2)  | Password hashing |
