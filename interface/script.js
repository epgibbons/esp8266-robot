$( document ).ready(function() {

    // Device
    var address = "192.168.1.95";
    var device = new Device(address);

    // Buttons
    $('#forward').mousedown(function() {
      device.callFunction("forward");
    });
    $('#forward').mouseup(function() {
      device.callFunction("stop");
    });

    $('#right').mousedown(function() {
      device.callFunction("right");
    });
    $('#right').mouseup(function() {
      device.callFunction("stop");
    });

    $('#left').mousedown(function() {
      device.callFunction("left");
    });
    $('#left').mouseup(function() {
      device.callFunction("stop");
    });

    $('#backward').mousedown(function() {
      device.callFunction("backward");
    });
    $('#backward').mouseup(function() {
			$.get("ajax_info.txt", function(data, status){
				$("#demo").text(data);
			})
      device.callFunction("stop");
    });

});

function loadDoc() {
			$.get("ajax_info.txt", function(data, status){
				$("#demo").text(data);
			})


}


