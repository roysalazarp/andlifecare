# andlifecare

An online healthcare platform for managing communication and paperwork between doctors and patients.

## Tech

This project is written in C because it provides fine-grained control over hardware resources, facilitating the development of [performance-aware software](https://www.youtube.com/watch?v=x2EOOJg8FkA).

It adheres to C89 for its simplicity and avoids using 3rd-party libraries in order to retain control—both key factors in managing projects as they grow.

## Architecture

### Consumer layer - `web` & `api` directories

Responsible for rendering responses to be sent back to the client. Passes data or receives data from the domain layer.

### Domain layer - `core` directory

The domain layers perform data validation, as well as the persistence or retrieval of data from the database and 3rd party services.

## Prod server config and tools

Debian 12

- Snap
- NGINX
- Certbot
- PostgreSQL 16
- libpq-dev
