#ifndef PIEZO_SEISMO_HUB_WEB_PAGE_cpp
#define PIEZO_SEISMO_HUB_WEB_PAGE_cpp

const char * htmlMessage = R"HTML(

<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <link referrerpolicy="no-referrer" rel="icon" href="http://miniTunes.cbmHome/favicon.ico">
    <style> 
      p{} .data { font-size: 24px; font-weight: bold; }
      .right { text-align: right; }
      table thead th { 
        font-weight: bold; 
        padding: 5px; 
        border-spacing: 5px; }
      thead { padding: 5px;
        background: #CFCFCF; 
        border-bottom: 3px solid #000000; }
      table { border: 2px solid black; }
      td {
          border: 1px solid gray; 
          padding: 5px; 
          border-spacing: 5px; }
    </style>
    
    <script src="//d3js.org/d3.v4.js"></script>
    <script src="https://www.gstatic.com/charts/loader.js"></script>

    <script type="text/javascript">

      var ws;
      var wsUri = "ws:";
      var loc = window.location;
      if (loc.protocol === "https:") { wsUri = "wss:"; }
      // wsUri += "//" + loc.host + loc.pathname.replace("simple","ws/simple");
      wsUri += "//" + loc.host + ":81/";            
            
      ws = new WebSocket ( wsUri );
      errorTime = 0;

      function wsConnect () {
        ws.onerror = function (error) {
          document.getElementById ( 'STATUS' ).innerHTML = "ERROR!!";
          document.getElementById ( 'STATUS' ).style = "background:pink; color:red";
          document.getElementById ("overlay").style.display = "block";
        };
        ws.onclose = function ( event ) {
          // event 1000 is clean; 1006 is "no close packet"
          if (event.wasClean) {
            document.getElementById ( 'STATUS' ).innerHTML = "Connection cleanly closed";
            // alert('[close] Connection closed cleanly, code=${event.code} reason=${event.reason}');
          } else {
            // e.g. server process killed or network down
            // event.code is usually 1006 in this case
            document.getElementById ( 'STATUS' ).innerHTML = "Connection broken";
            // alert('[close] Connection died');            
            report = 'Trying to (re)open a WebSocket connection...';
            console.log ( report );
            document.getElementById ( 'ERROR_MSG' ).innerHTML = report;
            errorTime = Date.now();
            ws = new WebSocket ( wsUri );
          }
          document.getElementById ( 'STATUS' ).style = "background:none; color:red";
          document.getElementById ("overlay").style.display = "block";

          // in case of lost connection tries to reconnect every 3 secs
          setTimeout ( wsConnect, 3000 );          
        }
        ws.onopen = function () {
          document.getElementById ( 'STATUS' ).innerHTML = "connected";
          document.getElementById ( 'STATUS' ).style = "background:none; color:black";
          ws.send ( "Open for data" );
        }
        ws.onmessage = function ( msg ) {
        
          document.getElementById ("overlay").style.display = "none";
          if ( ( Date.now() - errorTime ) > 5 * 60 * 1000 ) {
            document.getElementById ( 'ERROR_MSG' ).innerHTML = "";
          }

          try {             
            var jsonObj = JSON.parse ( msg.data );
            // document.getElementById ( 'ERROR_MSG' ).innerHTML = "";
            // errorTime = Date.now();
          }
          catch(err) {
            var report = err.message + ' >> ' + msg.data;
            if ( 0 && msg.data != 'JSON Parse error: Unexpected identifier "ws" ws text (non-JSON) message here' ) {
              // alert ( report );  // generates a synchronous popup msg
              document.getElementById ( 'ERROR_MSG' ).innerHTML = report;  // async - allows to press on
              errorTime = Date.now();
            }
            return;
          }
          
          // server need sends the whole bundle every time since it doesn't know when a 
          // browser opens a new instance of the page
          
          if ( jsonObj.hasOwnProperty ( 'PROGNAME' ) ) {
            document.getElementById ( 'TITLE' ).innerHTML = ( jsonObj.UNIQUE_TOKEN ).concat ( " Seismometer" );
            document.getElementById ( 'PROGNAME' ).innerHTML = jsonObj.PROGNAME;
            document.getElementById ( 'VERSION' ).innerHTML = jsonObj.VERSION;
            document.getElementById ( 'VERDATE' ).innerHTML = jsonObj.VERDATE;
            document.getElementById ( 'UNIQUE_TOKEN' ).innerHTML = jsonObj.UNIQUE_TOKEN;
            document.getElementById ( 'IP_STRING' ).innerHTML = jsonObj.IP_STRING;
            document.getElementById ( 'NETWORK_SSID' ).innerHTML = jsonObj.NETWORK_SSID;
            document.getElementById ( 'OFFERED_SSID' ).innerHTML = jsonObj.OFFERED_SSID;
            var temp = jsonObj.MDNS_ID;
            document.getElementById ( 'MDNS_ID' ).innerHTML = 
              ( ( temp == "" ) || ( temp == "<failure>" ) ) ? "<em>mDns failed</em>" : temp.concat ( ".local" );
            document.getElementById ( 'MQTT_TOPIC' ).innerHTML = jsonObj.MQTT_TOPIC;
            document.getElementById ( 'LAST_BOOT_AT' ).innerHTML = jsonObj.LAST_BOOT_AT;
            
            document.getElementById ( 'TIME' ).innerHTML = jsonObj.TIME;
            document.getElementById ( 'ENERGY' ).innerHTML = jsonObj.ENERGY;
            energyChart ( jsonObj.THRESHOLD, jsonObj.ENERGY, jsonObj.EWMA_6m, jsonObj.EWMA_1h );
            document.getElementById ( 'THRESHOLD' ).innerHTML = jsonObj.THRESHOLD;
            document.getElementById ( 'TRIGGERED' ).innerHTML = jsonObj.TRIGGERED != 0 ? "YES" : "NO";

          }  // got a good message
          
        }  // ws.onmessage
        
      }  // wsConnect

      function pageUnload () {
        document.getElementById ( 'STATUS' ).innerHTML = "page unloaded";
        document.getElementById ( 'STATUS' ).style = "background:none; color:red";
        document.getElementById ( 'overlay' ).style.display = "block";
      }
      
      google.charts.load('current',{packages:['corechart']}).then(function () {
        // google.charts.setOnLoadCallback(doChart);
        chartDataTable = new google.visualization.DataTable ({
          cols: [ {id: 'Index', type: 'datetime'}, 
                  {id: 'Energy', type: 'number', label: 'Energy'},
                  {id: 'EWMA_6m', type: 'number', label: 'EWMA_6m'}, 
                  {id: 'EWMA_1h', type: 'number', label: 'EWMA_1h'},
                  {id: 'Threshold', type: 'number', label: 'Threshold'} 
                   ]
        });
      });
      
      historyLength_hours = 12;
      //                                min sec   f
      maxArrLen = historyLength_hours * 60 * 60 / 5;
      
      function energyChart ( threshold, energyValue, ewma_6m, ewma_1h ) {
      
        // if ( typeof ( chartDataTable ) == "undefined" ) {
        //   chartDataTable = new google.visualization.DataTable ({
        //     cols: [ {id: 'Index', type: 'datetime'}, 
        //             {id: 'Energy', type: 'number', label: 'Energy'},
        //             {id: 'EWMA_6m', type: 'number', label: 'EWMA_6m'}, 
        //             {id: 'EWMA_1h', type: 'number', label: 'EWMA_1h'},
        //             {id: 'Threshold', type: 'number', label: 'Threshold'} 
        //              ]
        //   });
        // }
        // if ( chartDataTable.getNumberOfRows() >= maxArrLen) {
          // if we have one row, and that's the max, we remove one row
          // to make room for one new row
          rowsToBeAdded = 1;
          chartDataTable.removeRows ( 0, chartDataTable.getNumberOfRows() - maxArrLen + rowsToBeAdded );
        // }
        
        chartDataTable.addRow ( 
          [ new Date(), 
            Number ( energyValue ),
            Number ( ewma_6m ), 
            Number ( ewma_1h ),
            Number ( threshold )] );
      
        const options = {
          // width: 1200,
          height: 600,
          hAxis: {
            format: 'HH:mm:ss',
            gridlines: {count: 15},
            slantedText:true, 
            slantedTextAngle:60,
            title: historyLength_hours + " hours of history"
          },
          vAxis: {
            scaleType: 'log'
          },
          title: 'Energy',
          legend: {
            position: 'right',
            textStyle: { fontSize: 12 }
          },
          explorer: { 
            actions: ['dragToZoom', 'rightClickToReset'],
            keepInBounds: true },
          backgroundColor: '#fff8f8',
          series: {
            0: { color: 'blue',     lineWidth: 2                        },   // energy
            1: { color: '#6020cc',  lineWidth: 1, lineDashStyle: [4, 2] },   // EWMA_6m
            2: { color: '#2060cc',  lineWidth: 1, lineDashStyle: [8, 2] },   // EWMA_1h
            3: { color: 'red',      lineWidth: 2                        }    // threshold
          }
        };

        // const chart = new google.visualization.LineChart(document.getElementById('PLOT_FIG'));
        // chart.draw ( chartDataTable, options );
        
        // chartName = "Vibration_Energy";
        // if (charts[chartName] === undefined || charts[chartName] === null) {
        //   charts[chartName] = new google.visualization.LineChart ( document.getElementById('PLOT_FIG') );
        // } else {
        //   charts[chartName].clearChart();
        // }
        // charts[chartName].draw ( chartDataTable, options );

        // if (chart === undefined || chart === null) {
        //   chart = new google.visualization.LineChart ( document.getElementById('PLOT_FIG') );
        // } else {
        //   chart.clearChart();
        // }
        // chart.draw ( chartDataTable, options );

        try {
          // document.body.scrollTop and document.body.scrollLeft
          t = document.documentElement.scrollTop;
          console.log ( "scrollTop pre-clear: " + t );
          l = document.documentElement.scrollLeft;
          chart.clearChart();
          // the clear scrolls to the top because the page has shrunk!
          // so redraw the chart BEFORE resetting the scrolls
          // console.log ( "scrollTop post-clear: " + t );
          chart.draw ( chartDataTable, options );
          document.documentElement.scrollTop = t;
          document.documentElement.scrollLeft = l;
          // console.log ( "scrollTop after reset: " + t );
        } catch ( err ) {
          chart = new google.visualization.LineChart ( document.getElementById('PLOT_FIG') );
          chart.draw ( chartDataTable, options );
        }
      }

    </script>

    <title id="TITLE">Piezo Seismometer Hub</title>

  </head>


  <body onload="wsConnect()" onunload="pageUnload()">

    <div id="overlay" style="
        position: fixed; /* Sit on top of the page content */
        display: block; /* shown by default */
        width: 100%; /* Full width (cover the whole page) */
        height: 100%; /* Full height (cover the whole page) */
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0,0,0,0.25); /* background color, with opacity */
        z-index: 2; /* Specify a stack order in case you're using a different order for other elements */
        cursor: pointer; /* Add a pointer on hover */
    ">  
      <span style="
        position: absolute;
        top: 50%;
        left: 50%;
        font-size: 50px;
        background: none;
        color: red;
        transform: translate(-50%,-50%);
        -ms-transform: translate(-50%,-50%);
      ">
        <span id="STATUS">initializing...</span>
      </span>
    </div> 

    <header>
    <h1>Status: Piezo Seismometer Hub</h1>
      <h2>Assignment: Display seismometric activity</h2>
      <h3><span id="PROGNAME">..progname..</span> 
        v<span id="VERSION">..version..</span> 
        <span id="VERDATE">..verdate..</span> cbm
      </h3>
      
      <h4>    
         Running on 
            <span id="UNIQUE_TOKEN">..unique_token..</span> at 
            <span id="IP_STRING">..ipstring..</span> / 
            <span id="MDNS_ID">..mdns..</span> via access point 
            <span id="NETWORK_SSID">..network_id..</span>
        <br>
          Offering AP SSID &quot;<span id="OFFERED_SSID">..offered_ssid..</span>&quot;
        <br>
          MQTT topic: <em><span id="MQTT_TOPIC">..mqtt_topic..</span></em>
      </h4>
    </header>
    
    <br>
    <span id=ERROR_MSG></span>
    <table style="margin-top:20px; margin-left:10%;">
      <tr><td>Energy: </td><td><span id="ENERGY">..energy..</span></td></tr>
      <tr><td>Threshold: </td><td><span id="THRESHOLD">..threshold..</span></td></tr>
      <tr><td>Triggered: </td><td><span id="TRIGGERED">..triggered..</span></td></tr>
      <tr><td colspan="2" style="background-color:#e0e0e0; text-align:center;"> 
        At <span id="TIME">..time..</span>
      </td></tr>
    </table>
    <br>

    <div style="
        width: 80%;
        height: 60%;
        top: 0;
        left: 10%;
        right: 0;
        bottom: 0;
    ">
    <figure id="PLOT_FIG">
      <canvas id="SEISMO_CHART" <!-- style="height:50%; width:100%; max-width:1200px" --> ></canvas> 
        ..Plot goes here..
      <!-- <figcaption>About 10 minutes of vibration history</figcaption> -->
    </figure>
    </div>

    <br><br>Last boot at <span id="LAST_BOOT_AT">..last_boot..</span><br>

  </body>
</html>

)HTML";

#endif