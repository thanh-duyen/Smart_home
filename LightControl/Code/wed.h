#ifndef wed_h
#define wed_h

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
 #include "stdlib.h"
 #include "wiring.h"
#endif

char GiamSat[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        .button {
            border: none;
            color: white;
            height: 46px;
            text-align: center;
            text-decoration: none;
            display: inline-block;
            font-size: 14px;
            margin: 4px 0px;
            cursor: pointer;
        }

        .button30px {
            height: 30px;
        }

        .buttonGreen {
            background-color: #4CAF50;
        }

        .buttonGray {
            background-color: #414141;
        }

        .buttonBlue {
            background-color: #008CBA;
        }

        .buttonRed {
            background-color: #ff3333;
        }

        .fullScreen {
            width: 100%;
        }
        
        .a {
          background-color: #414141;
          color: white;
          height: 30.7px;
          text-align: center;
          vertical-align: middle;
          line-height: 28px;
          text-decoration: none;
          display: inline-block;
        }
        
        .btn-group button {
          background-color: #04AA6D; /* Green background */
          border: 1px solid green; /* Green border */
          color: white; /* White text */
          width: 25%;
          padding: 10px 5px; /* Some padding */
          cursor: pointer; /* Pointer/hand icon */
          margin: 5px 0px 0px 0px;
          float: left; /* Float the buttons side by side */
        }
        
        .btn-group:after {
          content: "";
          clear: both;
          display: table;
        }
        
        .btn-group button:not(:last-child) {
          border-right: none; /* Prevent double borders */
        }
        
        .btn-group button:hover {
          background-color: #3e8e41;
        }
    </style>
</head>

<body onload="javascript:init()">
    <div class="container-fluid">
        <div class="row">
            <div class="col-12">
                <center>
                    <b>Thời gian hoạt động: </b><b id="time">0</b><b>s</b>
                </center>
            </div>
        </div>
        <div class="row">
            <div class="btn-group">
                <p style="float: left; margin: 20px 0px 0px 0px; width: 13%;"><b>Relay 1:</b></p>
                <button onclick="USE(1)" style="width: 17%; margin: 10px 0px 0px 0px;" id="use1">Use</button>
                <input style="margin: 18px 0px 0px 1%; width: 25%; float: left;" type="text" id="name1" placeholder="Device name" onchange="change()">
                <button onclick="OFF(1)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #4CAF50;">OFF</button>
                <button onclick="ON(1)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #ff3333; float: right;">ON</button>
            </div>
        </div>
        <div class="row">
            <div class="btn-group">
                <p style="float: left; margin: 20px 0px 0px 0px; width: 13%;"><b>Relay 2:</b></p>
                <button onclick="USE(2)" style="width: 17%; margin: 10px 0px 0px 0px;" id="use2">Use</button>
                <input style="margin: 18px 0px 0px 1%; width: 25%; float: left;" type="text" id="name2" placeholder="Device name" onchange="change()">
                <button onclick="OFF(2)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #4CAF50;">OFF</button>
                <button onclick="ON(2)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #ff3333; float: right;">ON</button>
            </div>
        </div>
        <div class="row">
            <div class="btn-group">
                <p style="float: left; margin: 20px 0px 0px 0px; width: 13%;"><b>Relay 3:</b></p>
                <button onclick="USE(3)" style="width: 17%; margin: 10px 0px 0px 0px;" id="use3">Use</button>
                <input style="margin: 18px 0px 0px 1%; width: 25%; float: left;" type="text" id="name3" placeholder="Device name" onchange="change()">
                <button onclick="OFF(3)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #4CAF50;">OFF</button>
                <button onclick="ON(3)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #ff3333; float: right;">ON</button>
            </div>
        </div>
        <div class="row">
            <div class="btn-group">
                <p style="float: left; margin: 20px 0px 0px 0px; width: 13%;"><b>Relay 4:</b></p>
                <button onclick="USE(4)" style="width: 17%; margin: 10px 0px 0px 0px;" id="use4">Use</button>
                <input style="margin: 18px 0px 0px 1%; width: 25%; float: left;" type="text" id="name4" placeholder="Device name" onchange="change()">
                <button onclick="OFF(4)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #4CAF50;">OFF</button>
                <button onclick="ON(4)" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #ff3333; float: right;">ON</button>
            </div>
        </div>
        <div class="row">
            <div class="btn-group">
                <p style="float: left; margin: 20px 0px 0px 0px; width: 13%;"><b>Address:</b></p>
                <input style="margin: 20px 0px 0px 0px; width: 43%; float: left;" type="text" id="address" onchange="change()">
                <button onclick="done()" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #008CBA;" id="done">Done</button>
                <button onclick="save()" style="margin: 10px 0px 0px 1%; width: 20%; background-color: #4CAF50; float: right;" id="save">Save</button>
            </div>
        </div>
        
    </div>
</body>

</html>
<script>
    function init() {
        socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        socket.onmessage = function(event) {
            var dataTxtBuffer = event.data;
            if (dataTxtBuffer.indexOf("Saved") == 0) {
                document.getElementById("save").style.background='#4CAF50';
            }
            else if(dataTxtBuffer.indexOf("USE%") == 0){
                var id = ["use1","use2","use3","use4"];
                for(var i = 4; i < 8; i++){
                    if(dataTxtBuffer.charAt(i) == 0){
                        document.getElementById(id[i-4]).style.background='#414141';
                        document.getElementById(id[i-4]).textContent = "Un-used";
                    }
                    else{
                        document.getElementById(id[i-4]).style.background='#008CBA';
                        document.getElementById(id[i-4]).textContent = "Used";
                    }
                }
            }
            else if(dataTxtBuffer.indexOf("IN4%") == 0){
                var arr = dataTxtBuffer.split("%");
                var id = ["name1","name2","name3","name4"];
                document.getElementById("address").value = arr[1];
                document.getElementById(id[0]).value = arr[2];
                document.getElementById(id[1]).value = arr[3];
                document.getElementById(id[2]).value = arr[4];
                document.getElementById(id[3]).value = arr[5];
            }
            else if(dataTxtBuffer.indexOf("TM") == 0){
                dataTxtBuffer = dataTxtBuffer.replace("TM","");
                document.getElementById("time").textContent = dataTxtBuffer;
            }
            console.log(dataTxtBuffer);
        }
    }
    function ON(index){
        socket.send("ON" + index);
    }
    function change(){
        document.getElementById("save").style.background='#ff3333';
    }
    function OFF(index){
        socket.send("OFF" + index);
    }
    function USE(index){
        socket.send("USE" + index);
        change();
    }
    function done(){
        socket.send("Done");
        document.getElementById("done").style.background='#ff3333';
    }
    function save(){
        var i = 0;
        var buff = "IN4%";
        buff += document.getElementById("address").value+"%";
        var id = ["name1","name2","name3","name4"];
        while(i < 4){
          buff += document.getElementById(id[i]).value+"%";
          i++;
        }
        socket.send(buff);
    }
    
</script>
)=====";
#endif
