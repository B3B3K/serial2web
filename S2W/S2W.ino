//Author:  Osman Çelik
//Account: x.com/bruja_xH

//CHANGE WIFI PASS AND SSID

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

const char* ssid = "";
const char* password = "";

ESP8266WebServer server(80);

unsigned long baudRate = 115200;
uint8_t dataBits = 8;
String parity = "none";
uint8_t stopBits = 1;

String serialBuffer;
const size_t maxBufferSize = 8192; // Increased buffer for nano output

void setup() {
  Serial.begin(baudRate);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  server.on("/", handleRoot);
  server.on("/settings", handleSettingsPage);
  server.on("/data", handleData);
  server.on("/getsettings", handleGetSettings);
  server.on("/updatesettings", HTTP_POST, handleUpdateSettings);
  server.on("/clear", handleClear);
  server.on("/send", HTTP_POST, handleSendData);
  
  server.begin();
}

void loop() {
  server.handleClient();
  
  while (Serial.available()) {
    char c = Serial.read();
    serialBuffer += c;
    
    if (serialBuffer.length() > maxBufferSize) {
      serialBuffer = serialBuffer.substring(serialBuffer.length() - maxBufferSize/2);
    }
  }
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>ESP-01 Serial Monitor</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: monospace;
      background-color: #000;
      color: #ccc;
      margin: 0;
      padding: 10px;
    }
    #terminal {
      background-color: #000;
      color: #ccc;
      border: 1px solid #333;
      padding: 10px;
      height: 80vh;
      overflow-y: auto;
      white-space: pre;
      word-break: break-all;
      line-height: 1.2;
      font-size: 14px;
    }
    #input {
      width: 100%;
      background-color: #111;
      color: #fff;
      border: 1px solid #333;
      padding: 8px;
      font-family: monospace;
      margin-top: 5px;
    }
    .control-buttons {
      margin: 5px 0;
      display: flex;
      flex-wrap: wrap;
      gap: 5px;
    }
    button {
      background-color: #444;
      color: #fff;
      border: none;
      padding: 5px 10px;
      border-radius: 3px;
      cursor: pointer;
      min-width: 70px;
    }
    button:hover {
      background-color: #555;
    }
    a {
      color: #4af;
      text-decoration: none;
    }
    a:hover {
      text-decoration: underline;
    }
    .status-bar {
      background-color: #222;
      padding: 5px;
      margin-top: 5px;
      font-size: 12px;
    }
  </style>
</head>
<body>
  <h1>ESP-01 Serial Monitor</h1>
  <p><a href="/settings">Serial Settings</a></p>
  
  <div id="terminal"></div>
  
  <div class="control-buttons">
    <button onclick="sendSpecialChar('\\x03')">Ctrl+C</button>
    <button onclick="sendSpecialChar('\\x04')">Ctrl+D</button>
    <button onclick="sendSpecialChar('\\x18')">Ctrl+X</button>
    <button onclick="sendSpecialChar('\\x13')">Ctrl+S</button>
    <button onclick="sendSpecialChar('\\x1A')">Ctrl+Z</button>
    <button onclick="sendSpecialChar('\\x1B[A')">↑</button>
    <button onclick="sendSpecialChar('\\x1B[B')">↓</button>
    <button onclick="sendSpecialChar('\\x1B[C')">→</button>
    <button onclick="sendSpecialChar('\\x1B[D')">←</button>
    <button onclick="sendSpecialChar('\\x0D')">Enter</button>
    <button onclick="sendSpecialChar('\\x09')">Tab</button>
    <button onclick="clearTerminal()">Clear</button>
  </div>
  
  <input type="text" id="input" placeholder="Type here and press Enter to send" onkeydown="handleKeyDown(event)">
  
  <div class="status-bar" id="status"></div>
  
  <script>
    const terminal = document.getElementById('terminal');
    const input = document.getElementById('input');
    const statusBar = document.getElementById('status');
    let autoScroll = true;
    let lastDataLength = 0;
    
    // Enhanced ANSI sequence processing
    function processTerminalOutput(text) {
      // Remove bracketed paste mode sequences
      text = text.replace(/\x1b\[\?2004[hl]/g, '');
      
      // Remove cursor hide/show sequences
      text = text.replace(/\x1b\[\?25[hl]/g, '');
      
      // Remove screen clearing sequences
      text = text.replace(/\x1b\[[0-9]*[JK]/g, '');
      
      // Remove cursor position sequences
      text = text.replace(/\x1b\[[0-9;]*[Hf]/g, '');
      
      // Remove character attributes
      text = text.replace(/\x1b\[[0-9;]*m/g, '');
      
      // Remove nano specific sequences
      text = text.replace(/\x1b\[[0-9;]*r/g, ''); // Scroll region
      text = text.replace(/\x1b\[[0-9;]*[ABCDEFGJKSTfmnsu]/g, ''); // Other controls
      text = text.replace(/\x1b[\(\)B]/g, ''); // Character set selection
      text = text.replace(/\x1b\]0;[^\x07]*\x07/g, ''); // Window title
      
      // Convert line breaks for HTML
      text = text.replace(/\n/g, '<br>');
      text = text.replace(/\r/g, '');
      
      return text;
    }
    
    function fetchData() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          if (data) {
            const processedData = processTerminalOutput(data);
            
            // Only update if there's new data to improve performance
            if (processedData.length > 0) {
              terminal.innerHTML += processedData;
              
              // Update status bar with last received data info
              statusBar.textContent = `Received ${data.length} bytes | Total: ${terminal.textContent.length} chars`;
              
              if (autoScroll) {
                terminal.scrollTop = terminal.scrollHeight;
              }
            }
          }
        })
        .catch(error => {
          statusBar.textContent = `Error: ${error}`;
        });
    }
    
    setInterval(fetchData, 100);
    
    terminal.addEventListener('scroll', () => {
      autoScroll = terminal.scrollTop + terminal.clientHeight >= terminal.scrollHeight - 10;
    });
    
    function clearTerminal() {
      fetch('/clear')
        .then(() => {
          terminal.innerHTML = '';
          statusBar.textContent = 'Terminal cleared';
          setTimeout(() => statusBar.textContent = '', 2000);
        });
    }
    
    function sendData(data) {
      fetch('/send', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: 'data=' + encodeURIComponent(data)
      })
      .then(() => {
        statusBar.textContent = `Sent: ${data.length} bytes`;
        setTimeout(() => statusBar.textContent = '', 2000);
      });
    }
    
    function sendSpecialChar(escapedChar) {
      const char = new Function('return "' + escapedChar + '"')();
      sendData(char);
    }
    
    function handleKeyDown(event) {
      if (event.ctrlKey) {
        switch (event.key.toLowerCase()) {
          case 'x': sendSpecialChar('\\x18'); break;
          case 's': sendSpecialChar('\\x13'); break;
          case 'c': sendSpecialChar('\\x03'); break;
          case 'd': sendSpecialChar('\\x04'); break;
          case 'z': sendSpecialChar('\\x1A'); break;
          default: return; // Don't prevent default for other Ctrl combinations
        }
        event.preventDefault();
      } else if (event.key === 'Enter') {
        sendData(input.value + '\n');
        input.value = '';
        event.preventDefault();
      } else if (event.key === 'Tab') {
        sendSpecialChar('\\x09');
        event.preventDefault();
      } else if (event.key.startsWith('Arrow')) {
        const arrowMap = {
          'ArrowUp': 'A',
          'ArrowDown': 'B',
          'ArrowRight': 'C',
          'ArrowLeft': 'D'
        };
        sendSpecialChar('\\x1B[' + arrowMap[event.key]);
        event.preventDefault();
      }
    }
    
    // Focus input field on page load
    window.addEventListener('load', () => {
      input.focus();
    });
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleSettingsPage() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Serial Settings</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #222;
      color: #ddd;
      margin: 0;
      padding: 20px;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background-color: #333;
      padding: 20px;
      border-radius: 5px;
    }
    h1 {
      color: #4af;
      margin-top: 0;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
    }
    select, input {
      width: 100%;
      padding: 8px;
      background-color: #444;
      color: #fff;
      border: 1px solid #555;
      border-radius: 3px;
    }
    button {
      background-color: #4af;
      color: white;
      border: none;
      padding: 10px 15px;
      border-radius: 3px;
      cursor: pointer;
      font-weight: bold;
    }
    button:hover {
      background-color: #6cf;
    }
    .back-link {
      display: inline-block;
      margin-top: 15px;
      color: #4af;
      text-decoration: none;
    }
    .back-link:hover {
      text-decoration: underline;
    }
    .status {
      margin-top: 15px;
      padding: 10px;
      background-color: #444;
      border-radius: 3px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Serial Port Settings</h1>
    
    <form id="settingsForm">
      <div class="form-group">
        <label for="baud">Baud Rate:</label>
        <select id="baud">
          <option value="300">300</option>
          <option value="1200">1200</option>
          <option value="2400">2400</option>
          <option value="4800">4800</option>
          <option value="9600">9600</option>
          <option value="19200">19200</option>
          <option value="38400">38400</option>
          <option value="57600">57600</option>
          <option value="74880">74880</option>
          <option value="115200">115200</option>
        </select>
      </div>
      
      <div class="form-group">
        <label for="dataBits">Data Bits:</label>
        <select id="dataBits">
          <option value="5">5</option>
          <option value="6">6</option>
          <option value="7">7</option>
          <option value="8">8</option>
        </select>
      </div>
      
      <div class="form-group">
        <label for="parity">Parity:</label>
        <select id="parity">
          <option value="none">None</option>
          <option value="even">Even</option>
          <option value="odd">Odd</option>
        </select>
      </div>
      
      <div class="form-group">
        <label for="stopBits">Stop Bits:</label>
        <select id="stopBits">
          <option value="1">1</option>
          <option value="2">2</option>
        </select>
      </div>
      
      <button type="button" onclick="saveSettings()">Save Settings</button>
    </form>
    
    <a href="/" class="back-link">← Back to Terminal</a>
    
    <div id="status" class="status"></div>
  </div>
  
  <script>
    document.addEventListener('DOMContentLoaded', function() {
      fetch('/getsettings')
        .then(response => response.json())
        .then(settings => {
          document.getElementById('baud').value = settings.baudRate;
          document.getElementById('dataBits').value = settings.dataBits;
          document.getElementById('parity').value = settings.parity;
          document.getElementById('stopBits').value = settings.stopBits;
        });
    });
    
    function saveSettings() {
      const settings = {
        baudRate: document.getElementById('baud').value,
        dataBits: document.getElementById('dataBits').value,
        parity: document.getElementById('parity').value,
        stopBits: document.getElementById('stopBits').value
      };
      
      const status = document.getElementById('status');
      status.textContent = "Saving settings...";
      
      fetch('/updatesettings', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `baud=${settings.baudRate}&dataBits=${settings.dataBits}&parity=${settings.parity}&stopBits=${settings.stopBits}`
      })
      .then(response => response.text())
      .then(message => {
        status.textContent = message;
        setTimeout(() => status.textContent = '', 3000);
      })
      .catch(error => {
        status.textContent = "Error: " + error;
      });
    }
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleData() {
  server.send(200, "text/plain", serialBuffer);
  serialBuffer = "";
}

void handleGetSettings() {
  String json = "{";
  json += "\"baudRate\":" + String(baudRate) + ",";
  json += "\"dataBits\":" + String(dataBits) + ",";
  json += "\"parity\":\"" + parity + "\",";
  json += "\"stopBits\":" + String(stopBits);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleUpdateSettings() {
  if (server.hasArg("baud")) {
    baudRate = server.arg("baud").toInt();
  }
  if (server.hasArg("dataBits")) {
    dataBits = server.arg("dataBits").toInt();
  }
  if (server.hasArg("parity")) {
    parity = server.arg("parity");
  }
  if (server.hasArg("stopBits")) {
    stopBits = server.arg("stopBits").toInt();
  }
  
  Serial.end();
  Serial.begin(baudRate);
  
  server.send(200, "text/plain", "Settings updated successfully");
}

void handleClear() {
  serialBuffer = "";
  server.send(200, "text/plain", "Terminal cleared");
}

void handleSendData() {
  if (server.hasArg("data")) {
    String data = server.arg("data");
    String decodedData;
    
    // Process escape sequences
    for (size_t i = 0; i < data.length(); i++) {
      if (data[i] == '\\' && i + 1 < data.length()) {
        switch (data[i+1]) {
          case 'x': // Hex sequence
            if (i + 3 < data.length()) {
              char hex[3] = { data[i+2], data[i+3], '\0' };
              decodedData += (char)strtol(hex, NULL, 16);
              i += 3;
            }
            break;
          case 'n': decodedData += '\n'; i++; break;
          case 'r': decodedData += '\r'; i++; break;
          case 't': decodedData += '\t'; i++; break;
          case '\\': decodedData += '\\'; i++; break;
          default: decodedData += data[i+1]; i++; break;
        }
      } else {
        decodedData += data[i];
      }
    }
    
    Serial.print(decodedData);
    server.send(200, "text/plain", "Data sent: " + String(decodedData.length()) + " bytes");
  } else {
    server.send(400, "text/plain", "No data provided");
  }
}
