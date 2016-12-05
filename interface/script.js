function loadLatestResults(){

	$.get("http://192.168.1.95/variable",{name:"latitude"}).done( function(data ){
		$("#latitude").text(data);
	});
	$.get("http://192.168.1.95/variable",{name:"longitude"}).done( function(data ){
		$("#longitude").text(data);
	});
	$.get("http://192.168.1.95/variable",{name:"headingDegrees"}).done( function(data ){
		$("#headingDegrees").text(data);
	});
	$.get("http://192.168.1.95/variable",{name:"accelX"}).done( function(data ){
		$("#accelX").text(data);
	});
	$.get("http://192.168.1.95/variable",{name:"accelY"}).done( function(data ){
		$("#accelY").text(data);
	});
	$.get("http://192.168.1.95/variable",{name:"accelZ"}).done( function(data ){
		$("#accelZ").text(data);
	});
}


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
      device.callFunction("stop");
    });
		
    $('#displayReadings').mousedown(function(){
			loadLatestResults();
		});

/* 		window.setInterval(function(){
			loadLatestResults();
		}, 2000); */
		
		
	} //end of ready function
	); //end of ready			
			




