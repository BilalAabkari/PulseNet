# PulseNet
A distributed realtime database and events bus service

## Requirements for building the project

* cmake
* g++ compiler


## Postgres DB setup
This project uses Postgre database. Here are the instructions for debian based distributions (ubuntu, lubuntu, debian...):

* Install postgre:

`sudo apt install postgresql postgresql-contrib`

* Install some dependencies:

`sudo apt install libpq-dev`

*  Create a role for your user:

`sudo -u postgres createuser --interactive` and follow the instructions

*  Set a password for default db


You need a password for the default Postgre DB:

```
sudo -i -u postgres
psql
ALTER USER postgres WITH PASSWORD 'your_password';
```

