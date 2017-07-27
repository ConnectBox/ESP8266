/*

  This is an attempt to create a ESP8266 / WEMOS D1 Mini version of the ConnectBox.
  
  The code below is based on the SDWebServer and CaptivePortal example code.

  Have a FAT Formatted SD Card connected to the SPI port of the ESP8266
  The web root is the SD Card root folder
  
  File extensions with more than 3 charecters are not supported by the SD Library
  File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter
  index.html is the default index (works on subfolders as well)

*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <SPI.h>
#include <SD.h>

#define DBG_OUTPUT_PORT Serial
#define DEBUG_WEBSERVER

const char* ssid = "ConnectBox-Mini";
const char* host = "ConnectBox";

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);

static bool hasSD = false;
File uploadFile;

const int chipSelect = D8;

void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadIndex(){
     loadFromSdCard("index.html");
}

bool loadFromSdCard(String path){
  DBG_OUTPUT_PORT.println("loadFromSdCard " + path);

  if(!SD.exists(path)){
    DBG_OUTPUT_PORT.println("Not found. Sending 404");
    server.send(404, "text/plain", "File Not Found " + server.uri());
  }
  
  String dataType = "text/plain";
  dataType = getContentType(path);

  DBG_OUTPUT_PORT.println("Serving file " + path + " " + dataType);
  File dataFile = SD.open(path);
  if(dataFile.isDirectory()){
    DBG_OUTPUT_PORT.println("Path is a directory");
    printDirectory(path);
  }

  if (!dataFile){
    DBG_OUTPUT_PORT.println("not a data file");
    return false;
  }

  DBG_OUTPUT_PORT.println("About to send the file.");
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void printDirectory(String path) {
  DBG_OUTPUT_PORT.println("printDirectory " + path);
  
  if(!SD.exists(path)) return returnFail(path);
  File dir = SD.open(path);
 
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();

  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry)
    break;
    
    String output;
    if (cnt > 0)
      output = ',';

    output += "{\"name\":\"";
    output += entry.name();
  
    output += "\",\"type\":\"";
    output += (entry.isDirectory()) ? "directory" : "file";
    
    output += "\",\"mtime\":\"";
  
    if (!entry.isDirectory()) {
      output += "\",\"size\":\"";
      output += entry.size();
    }
    
    output += "\"";
    output += "}";
    
    server.sendContent(output);
    entry.close();
 }
 server.sendContent("]");
 dir.close();
}

String getContentType(String filename) {
  if (server.hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  }
  else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  }
  else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
  else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

void setup(void){
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid);

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }

  server.on("/index.html", HTTP_GET, loadIndex);
  server.on("/", HTTP_GET, loadIndex);
  //server.onNotFound( [] () { loadFromSdCard(server.uri()); });

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");
  
  if (SD.begin(chipSelect)){
     DBG_OUTPUT_PORT.println("SD Card initialized.");
     hasSD = true;
  }
}

void loop(void){
  dnsServer.processNextRequest();
  server.handleClient();
}
