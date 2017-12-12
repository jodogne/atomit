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
  $('#chart').attr('href', 'chart.html?id=' + encodeURIComponent(series));

  $.ajax({
    url: '../series/' + series + '/content?limit=0',
    cache: false,
    success: function(data) {
      var source = [];

      for (var i = 0; i < data.content.length; i++) {
        var item = data.content[i];

        var value;

        if (item.value.length >= 128)
        {
          value = '(too long)';
        }
        else
        {
          value = item.value;

          if (item.base64)
          {
            value += ' (base64)';
          }
        }
        
        var line = [ item.timestamp,
                     item.metadata,
                     value ];
        
        source.push(line);
      }

      $('#content').DataTable({
        data: source,
        columns: [
          { title: 'Timestamp' },
          { title: 'Metadata' },
          { title: 'Value' }
        ],
        'pageLength': 25
      });      
    }
  });


});
