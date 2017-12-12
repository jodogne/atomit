CREATE TABLE GlobalProperties(
       property INTEGER PRIMARY KEY,
       value TEXT
       );

CREATE TABLE TimeSeries(
       internalId INTEGER PRIMARY KEY AUTOINCREMENT,
       publicId TEXT,
       maxLength INTEGER,
       maxSize INTEGER,
       currentLength INTEGER,
       currentSize INTEGER,
       lastTimestamp INTEGER
       );

CREATE TABLE Content(
       id INTEGER REFERENCES TimeSeries(internalId) ON DELETE CASCADE,
       timestamp INTEGER,
       size INTEGER,
       metadata TEXT,
       value TEXT,
       PRIMARY KEY(id, timestamp)
       );
       
CREATE INDEX TimeSeriesIndex ON TimeSeries(publicId);
