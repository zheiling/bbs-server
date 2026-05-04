# Development plans

- [ ] Remove file when error while saving the data in DB

## Tasks

- [ ] Switch to SQLite
- [ ] Files search doesn't work
- [ ] Check registration
- [ ] Common functions for both DBs, only db_query should be different
- [ ] Close the DB correctly in any quit case (also by SIGTERM)
- [x] Auto create a database when one is missing.

## Features

- [x] Database integration
- [x] Paging in the file list
- [x] Get files in FS by their hash
- [x] Registration
- [x] Encrypt passwords in DB
- [x] Registration: Error when username or email is already used
- [ ] Check correctness of email
- [ ] Message to admin
- [x] Advanced folder structure in the storage
- [ ] File encryption
- [ ] Connection encryption
- [ ] Advanced rights for files
- [ ] Multithreading