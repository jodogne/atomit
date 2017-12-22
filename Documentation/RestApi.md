REST API
========

The Atom-IT server comes bundled with a simple REST API that enables a
programmatic access to the time series from external applications
(scripts, heavyweight clients, Web applications...).  This REST API is
served through an embedded Web server, whose configuration can be
[fine-tuned](Configuration.md#web-server-parameters).

NB: The actual handling the REST API is carried by the
[`AtomIT::AtomITRestApi` class](../Applications/AtomITRestApi.cpp).


## `GET /app/*`

Returns the static resources implementing the [basic Web
interface](QuickStart.md#plot-the-time-series) of the Atom-IT server
(HTML and JavaScript files).


## `GET /series`

Returns the list of the time series stored in the Atom-IT server,
as a JSON array.


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


## `GET /series/{name}/content`

Returns the content of the time series whose identifier is `name`.
