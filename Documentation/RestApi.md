REST API
========

The Atom-IT server comes bundled with a minimalist REST API that
enables a programmatic access to the time series from external
applications (scripts, heavyweight clients, Web applications...).
This REST API is served through an embedded Web server, whose
configuration can be
[fine-tuned](Configuration.md#web-server-parameters).

NB: The actual handling of the REST API is carried by the
[`AtomIT::AtomITRestApi` class](../Applications/AtomITRestApi.cpp).


## `GET /app/*`

Returns the static resources implementing the [basic Web
interface](QuickStart.md#plot-the-time-series) of the Atom-IT server
(HTML and JavaScript files).


## `GET /series`

Returns the list of the time series stored in the Atom-IT server,
as a JSON array.

**Example:**

```
$ curl -u atomit:atomit http://localhost:8042/series/
[ "random", "sample" ]
```


## `POST /series/{name}`

Publishes one message to the time series whose identifier is
`name`:

 * The value of the message is set to the body of the HTTP POST
   request.
 * The metadata of the message is set to the [HTTP
   Content-Type](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type)
   of the request.
 * The timestamp is generated according to the [default timestamp
   policy](Configuration.md#timestamps-policy) of the target time series.

**Example:**

```
$ curl -u atomit:atomit -X POST http://localhost:8042/series/sample \
  -H 'Content-Type: text/plain' -d 'Hello world'
```


## `DELETE /series/{name}/content`

Remove all the messages from the time series whose identifier is
`name`.

**Example:**

```
$ curl -u atomit:atomit -X DELETE http://localhost:8042/series/sample/content
```


## `GET /series/{name}/content`

Returns the content of the time series whose identifier is `name`.

**Optional GET arguments:**

 * `since`: Specify the timestamp from which to retrieve the messages (for paging data).
 * `limit`: The maximum number of messages to be returned.
 * `last`: Ask to retrieve the last message of the time series.

**JSON return value:**
 
 * The `content` field is a JSON array containing the retrieved
   messages. Metadata and timestamp are readily available. The `value`
   field contains the value of the message, possibly Base64-encoded if
   the `base64` field is `true` (which indicates a binary value).
 * The `done` field is `true` if the last item of the `content` field is
   the last message (i.e. most recent) of the time series.
 * The `name` field duplicates the name of the time series.

**Examples:**

 * To return at most 2 messages starting at timestamp 10:

```
$ curl -u atomit:atomit 'http://localhost:8042/series/random/content?since=10&limit=2'
{
   "content" : [
      {
         "base64" : false,
         "metadata" : "application/x-www-form-urlencoded",
         "timestamp" : 10,
         "value" : "16.7206830128446"
      },
      {
         "base64" : false,
         "metadata" : "application/x-www-form-urlencoded",
         "timestamp" : 11,
         "value" : "95.4910972844552"
      }
   ],
   "done" : false,
   "name" : "random"
}
```

 * To retrieve the last message (i.e. the most recent message):


```
$ curl -u atomit:atomit 'http://localhost:8042/series/random/content?last'
{
   "content" : [
      {
         "base64" : false,
         "metadata" : "application/x-www-form-urlencoded",
         "timestamp" : 98,
         "value" : "12.2357817454041"
      }
   ],
   "done" : true,
   "name" : "random"
}
```


## `DELETE /series/{name}/content/{timestamp}`

Remove the message whose timestamp equals `timestamp`, from the time
series whose identifier is `name`.

**Example:**

```
$ curl -u atomit:atomit -X DELETE http://localhost:8042/series/random/content/50
```


## `GET /series/{name}/content/{timestamp}`

Get the value of the message whose timestamp equals `timestamp`, from
the time series whose identifier is `name`. The HTTP header
Content-Type of the answer is set to the metadata of the answer.

**Example:**

```
$ curl -u atomit:atomit http://localhost:8042/series/sample/content/0 -v
[...]
< HTTP/1.1 200 OK
< Content-Type: text/plain
< Content-Length: 11
< 
* Connection #0 to host localhost left intact
Hello world
```


## `PUT /series/{name}/content/{timestamp}`

Assign a message to some timestamp, in the time series whose identifier is `name`.
The `timestamp` must be after the last message of the time series in order to
fulfill the [CRD way of operations](Concepts.md#time-series).

**Example:**

```
$ curl -u atomit:atomit -X PUT http://localhost:8042/series/sample/content/42 \
  -H 'Content-Type: text/plain' -d 'Hello world'
```


## `GET /series/{name}/statistics`

Returns some statistics of the time series whose identifier is `name`.

**Example:**

```
$ curl -u atomit:atomit http://localhost:8042/series/sample/statistics
{
   "length" : 2,
   "name" : "sample",
   "size" : "22",
   "sizeMB" : 0
}
```

