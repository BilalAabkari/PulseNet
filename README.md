# PulseNet
A distributed realtime database and events bus service

## How to install:

install cmake

install g++

## Postgres DB setup

sudo apt install postgresql postgresql-contrib

sudo apt install libpq-dev

### Create a role:

sudo -u postgres createuser --interactive

### Set a password for default db

sudo -i -u postgres

psql

ALTER USER postgres WITH PASSWORD 'your_password';


