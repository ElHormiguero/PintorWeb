<!DOCTYPE html>
<html>
<head>
<title>Arduino - PHPoC Shield - XY Plotter</title>
<meta name="viewport" content="width=device-width, initial-scale=0.7">
<style>
body { text-align: center; background-color: #33C7F2; }
#canvas { margin-right: auto;
	margin-left: auto; 
	position: relative;
	background-color: #FFFFFF; 
}
canvas {
	position: absolute; 
	left: 0px;
	top: 0px;
	overflow-y: auto;
	overflow-x: hidden;
	-webkit-overflow-scrolling: touch; /* nice webkit native scroll */
}
#layer1 { z-index: 2; }
#layer2 { z-index: 1; }
#layer3 { z-index: 0; }
</style>
<script>
var PEN_STATE_UP = 0;
var PEN_STATE_DOWN = 1;
var CMD_PEN_UP = 0;
var CMD_PEN_DOWN = 1;
var CMD_MOVE = 2;
var RESOLUTION = 1000;
var MIN_UPDATE_INTERVAL = 100; // in millisecond
var MAX_X = 55550, MAX_Y = 68780;
var CANVAS_WIDTH = 0, CANVAS_HEIGHT = 0;

var ws = null;
var canvas = null;
var layer1 = null, layer2 = null, layer3 = null;
var ctx1 = null, ctx2 = null, ctx3 = null;

var touchState = 0;
var touchStepX = 0, touchStepY = 0; /* unit is step */
var touchPixelX = 0, touchPixelY = 0; /* unit is pixel */

var plotterX = 0, plotterY = 0;
var plotterPenState = PEN_STATE_UP;

var lastUpdateMillis;

var buffer = "";

function init() {
	canvas = document.getElementById("canvas");

	layer1 = document.getElementById("layer1"); /* for drawing current postion, which is received from XY plotter */
	layer2 = document.getElementById("layer2"); /* for drawing trajatory of XY plotter, which is received from XY plotter*/
	layer3 = document.getElementById("layer3"); /* for drawing the current touch postion, which is inputed by user on screen */
	ctx1 = layer1.getContext("2d");
	ctx2 = layer2.getContext("2d");
	ctx3 = layer3.getContext("2d");

	layer1.addEventListener("touchstart", mouse_down);
	layer1.addEventListener("touchend", mouse_up);
	layer1.addEventListener("touchmove", mouse_move);
	layer1.addEventListener("mousedown", mouse_down);
	layer1.addEventListener("mouseup", mouse_up);
	layer1.addEventListener("mousemove", mouse_move);

	canvasResize();

	var d = new Date();
	lastUpdateMillis = d.getTime();
}
function ws_onmessage(e_msg) {
	buffer += e_msg.data;

	var pos = buffer.indexOf('\n');

	if(pos == -1)
		return;

	var data = buffer.substring(0, pos);
	buffer = buffer.substring(pos + 1);

	var arr = JSON.parse(data);
	plotterX	= arr[0];
	plotterY	= arr[1];
	var newPlotterPenState	= arr[2];

	/* convert step unit to pixel unit */
	plotterX = Math.round(plotterX * CANVAS_WIDTH / MAX_X);
	plotterY = -Math.round(plotterY * CANVAS_HEIGHT / MAX_Y);

	/* draw current postion of plotter*/
	ctx1.clearRect(0, 0, CANVAS_WIDTH, -CANVAS_HEIGHT);
	ctx1.beginPath();
	ctx1.arc(plotterX, plotterY, 7, 0, 2*Math.PI);
	ctx1.fill();

	/* draw trajatory of plotter only when pen down */
	if(plotterPenState == PEN_STATE_UP && newPlotterPenState == PEN_STATE_DOWN)
		ctx2.beginPath();

	if(newPlotterPenState == PEN_STATE_DOWN) {
		ctx2.lineTo(plotterX, plotterY);
		ctx2.stroke();
	}

	plotterPenState = newPlotterPenState;
}
function ws_onopen() {
	document.getElementById("ws_state").innerHTML = "OPEN";
	document.getElementById("wc_conn").innerHTML = "Disconnect";
}
function ws_onclose() {
	document.getElementById("ws_state").innerHTML = "CLOSED";
	document.getElementById("wc_conn").innerHTML = "Connect";
	ws.onopen = null;
	ws.onclose = null;
	ws.onmessage = null;
	ws = null;
}
function wc_onclick() {
	if(ws == null) {
		ws = new WebSocket("ws://<?echo _SERVER("HTTP_HOST")?>/xy_plotter", "text.phpoc");
		document.getElementById("ws_state").innerHTML = "CONNECTING";

		ws.onopen = ws_onopen;
		ws.onclose = ws_onclose;
		ws.onmessage = ws_onmessage;  
	}
	else
		ws.close();
}
function event_handler(event, type) {
	// convert coordinate
	if(event.targetTouches) {
		if(event.targetTouches.length > 1)
			return false;

		touchPixelX = event.targetTouches[0].pageX - canvas.offsetLeft;
		touchPixelY = event.targetTouches[0].pageY - canvas.offsetTop - CANVAS_HEIGHT;
	} else {
		touchPixelX = event.offsetX;
		touchPixelY = event.offsetY - CANVAS_HEIGHT;
	}

	/* convert from pixel to step */
	var newTouchStepX = Math.round(touchPixelX / CANVAS_WIDTH * MAX_X);
	var newTouchStepY = Math.round((-touchPixelY) / CANVAS_HEIGHT * MAX_Y); 

	if(type == "MOVE") { /* check update condition to avoid sending too much data to plotter */
		var isUpdate = false;

		var d = new Date();
		var curMillis = d.getTime();

		if((curMillis - lastUpdateMillis) > MIN_UPDATE_INTERVAL) {
			isUpdate = true;
		}

		var deltaX = newTouchStepX - touchStepX;
		var deltaY = newTouchStepY - touchStepY;
		var dist = Math.sqrt( Math.pow(deltaX, 2) + Math.pow(deltaY, 2) );

		if(dist > RESOLUTION) 
			isUpdate = true;

		if(isUpdate == false)
			return false;

		lastUpdateMillis = curMillis;
	}

	touchStepX = newTouchStepX;
	touchStepY = newTouchStepY;

	drawXYline();

	return true;
}
function mouse_down() {
	event.preventDefault();
	event_handler(event, "DOWN");
	sendToPlotter(CMD_PEN_DOWN, touchStepX, touchStepY);
	touchState = 1;
}
function mouse_up() {
	event.preventDefault();
	touchState = 0;
	sendToPlotter(CMD_PEN_UP, touchStepX, touchStepY);
}
function mouse_move() {
	event.preventDefault();

	if(touchState == 1) {
		if(event_handler(event, "MOVE"))
			sendToPlotter(CMD_MOVE, touchStepX, touchStepY);
	}
}
function sendToPlotter(cmd, x, y) {
	if(ws != null && ws.readyState == 1)
		ws.send(cmd + ":" + touchStepX + ":" + touchStepY + "\r\n"); 
}
function canvasClear() {
	ctx2.clearRect(0, 0, CANVAS_WIDTH, -CANVAS_HEIGHT); /* only clear trajatory */
}
function canvasResize() {
	var width = Math.round(window.innerWidth*0.95);
	var height = Math.round(window.innerHeight*0.95) - 100;

	var temp_height = Math.round(width*MAX_Y/MAX_X);
	if(temp_height <= height) {
		CANVAS_WIDTH = width;
		CANVAS_HEIGHT = temp_height;
	} else {
		CANVAS_WIDTH = height*MAX_X/MAX_Y;
		CANVAS_HEIGHT = height;
	}

	canvas.style.width = CANVAS_WIDTH + "px";
	canvas.style.height = CANVAS_HEIGHT + "px";

	layer1.width = CANVAS_WIDTH;
	layer1.height = CANVAS_HEIGHT;
	layer2.width = CANVAS_WIDTH;
	layer2.height = CANVAS_HEIGHT;
	layer3.width = CANVAS_WIDTH;
	layer3.height = CANVAS_HEIGHT;

	ctx1.translate(0, CANVAS_HEIGHT);
	ctx2.translate(0, CANVAS_HEIGHT);
	ctx3.translate(0, CANVAS_HEIGHT);

	ctx1.fillStyle = "#00979d";
	ctx2.lineWidth = 3;
	ctx2.strokeStyle = "black";
	ctx3.strokeStyle = "00FFFF";
}
function drawXYline() {
	ctx3.clearRect(0, 0, CANVAS_WIDTH, -CANVAS_HEIGHT);
	ctx3.beginPath();
	ctx3.moveTo(0, touchPixelY);
	ctx3.lineTo(CANVAS_WIDTH, touchPixelY);
	ctx3.moveTo(touchPixelX, 0);
	ctx3.lineTo(touchPixelX, -CANVAS_HEIGHT);
	ctx3.stroke();
}
window.onload = init;
</script>
</head>

<body onresize="canvasResize()">
<br>
<div id="canvas">
	<canvas id="layer1"></canvas>
	<canvas id="layer2"></canvas>
	<canvas id="layer3"></canvas>
</div>
<p>WebSocket : <span id="ws_state">null</span></p>
<button id="wc_conn" type="button" onclick="wc_onclick();">Connect</button>
<button type="button" onclick="canvasClear();">Clear</button>
</body>

</html>