var host = 'http://photon1.internal.redmountainmakers.org';

function doRequest(method, path, cb) {
    $.ajax({
        url: host + path,
        method: method,
        dataType: 'json',
        error: function(xhr, status, error) {
            console.log('AJAX error', xhr, status, error);
            cb(error || new Error('AJAX error'));
        },
        success: function(data, status, xhr) {
            if (data.status === 'ok') {
                console.log('AJAX success', data);
                cb(null, data);
            } else {
                console.log('Device error', data);
                cb(new Error(data.message));
            }
        }
    });
}

$(function() {
    if (/hide_footer/.test(document.location.search)) {
        $('#footer').hide();
    }

    doRequest('GET', '/', function(err, data) {
        if (err) {

            // Try again for good measure
            doRequest('GET', '/', function(err, data) {
                if (err) {
                    // This is looking more like a real failure
                    $('#error').show();
                    $('#status-container, #controls').hide();
                } else {
                    $('#status').html(data.lights);
                }
            });

        } else {
            $('#status').html(data.lights);
        }
    });

    $('#controls').on('click', '.control', function() {
        doRequest('POST', $(this).data('route'), function(err, data) {
            if (err) {
                // There are some intermittent errors here, so don't hide the controls
                $('#error').show();
                $('#status-container').hide();
            } else {
                $('#error').hide();
                $('#status-container').show();
                $('#status').html(data.lights);
            }
        });
    });
});
