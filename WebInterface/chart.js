function GetArgument(name) {
  var results = new RegExp('[\?&]' + name + '=([^]*)').exec(window.location.href);
  if (results == null){
    return null;
  }
  else {
    return results[1] || 0;
  }
}

$(document).ready(function() {
  var series = GetArgument('id');
  $('#series').text(series);
  $('#raw').attr('href', 'content.html?id=' + encodeURIComponent(series));

  $.ajax({
    url: '../series/' + series + '/content?limit=0',
    cache: false,
    success: function(data) {
      var source = [];

      for (var i = 0; i < data.content.length; i++) {
        var item = data.content[i];

        if (!item.binary) {
          var value = parseFloat(item.value);
          if (!isNaN(value)) {        
            source.push([ item.timestamp, value ]);
          }
        }
      }

      if (source.length == 0) {
        alert('This time series has no numeric payload!');
      }

      // http://dygraphs.com/data.html#array
      g = new Dygraph(document.getElementById('content'),
                      source, {
                        labels: [ 'Timestamp', 'Value' ]
                        //legend: 'always',
                        //title: 'Values',
                        //showRoller: true,
                        //rollPeriod: 14,
                        //customBars: true,
                        //ylabel: 'Temperature (F)'
                      });
    }
  });

});
