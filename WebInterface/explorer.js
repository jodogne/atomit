$(document).ready(function() {

  $.ajax({
    url: '../series',
    cache: false,
    success: function(series) {
      var data = [];
      
      $.each(series.sort(), function(index, value) {

        $.ajax({
          url: '../series/' + value + '/statistics',
          cache: false,
          async: false,
          success: function(statistics) {
            data.push([
              value,
              statistics.length,
              statistics.size,
              value,
              value
            ]);
            
            $('#series').append($('<h2>').text('Time series: ' + value));
            var ul = $('<ul>');
            ul.append($('<li>').text('Number of items: ' + statistics.length));
            ul.append($('<li>').text('Size on disk: ' + statistics.size + ' bytes'));
            ul.append($('<li>').append($('<a>')
                                       .attr('href', 'content.html?id=' + value)
                                       .text('Open content')));
            ul.append($('<li>').append($('<a>')
                                       .attr('href', 'chart.html?id=' + value)
                                       .text('Plot time series')));
            $('#series').append(ul);
          }
        });
      });

      $('#content').DataTable({
        data: data,
        columns: [
          { title: 'Time series' },
          { title: 'Length' },
          { title: 'Size on disk' },
          { title: 'Open content' },
          { title: 'Plot time series' }
        ],
        'pageLength': 25,
        'columnDefs': [
          { 'className': 'dt-center', 'targets': '_all' },
          {
            targets: 3,
            render: function(data, type, row, meta) {
              return '<a href="content.html?id=' + encodeURIComponent(data) + '">Open</a>';
            }
          },
          {
            targets: 4,
            render: function(data, type, row, meta) {
              return '<a href="chart.html?id=' + encodeURIComponent(data) + '">Plot</a>';
            }
          }
        ]
      });
    }
  });
});
