- Generate a file. 10GB, 100GB.
- Run RDF code to read into record and write out to a different file (basically a copy).

- Could have a little helper that checks the timestamp of the file every second, and another process that updates the file.
  - Modifying existing record within file is slightly trickier.
  - But appending new record (remember record start point in file).

- Something that simulates a table server thread.
- Something that simulates async fetches etc. of table data source.