Samples
-------

Here is a sample configuration file illustrating how to create a
simple IoT workflow collecting its data from The Things Network using
MQTT, decoding the payload, and mapping it as a REST API onto URL
`http://localhost:8042/series/decoded/`:

```
{
  "AutoTimeSeries" : {},
  "Filters" : [
    {
      "Type" : "MQTTSource",
      "Output" : "source",
      "Broker" : {
        "Server" : "eu.thethings.network",
        "Username" : "XXX",
        "Password" : "ttn-account-v2.XXX"
      },
      "Topics" : [
        "+/devices/+/up"
      ]
    },
    {
      "Type" : "Lua",
      "Path" : "ParseTheThingsNetwork.lua",
      "Input" : "source",
      "Output" : "decoded"
    }
  ]
}
```

The `ParseTheThingsNetwork.lua` Lua script implements the decoding of
the payload as follows:

```
function Convert(timestamp, metadata, rawValue)
  local value = ParseJson(rawValue)
  local device = value['hardware_serial']
  local payload = DecodeBase64(value['payload_raw'])
  
  local result = {}
  result['metadata'] = device
  result['value'] = payload

  return result
end
```

More samples will come in the following days.
