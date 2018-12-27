//
// Created by scottw on 12/26/18.
//

//<editor-fold desc="includes">
#if defined(ARDUINO)
#if ARDUINO < 100
    #include <WProgram.h>
  #else
    #include <Arduino.h>
  #endif
#else
#include <Arduino.h>
#endif
#ifndef HASH_H_
#include <Hash.h>
#endif
#ifndef WiFi_h
#include <ESP8266WiFi.h>
#endif
#ifndef wificlient_h
#include <WiFiClient.h>
#endif
#ifndef _ESPAsyncWebServer_H_
#include <ESPAsyncWebServer.h>
#endif
#ifndef USEMDNS
#define USEMDNS 0
#endif
#if USEMDNS > 0
#ifndef ESP8266MDNS_H
#include <ESP8266mDNS.h>
#endif
#endif
#ifndef USEOTA
#define USEOTA 0
#endif
#if USEOTA > 0
#ifndef __ARDUINO_OTA_H
#include <ArduinoOTA.h>
#endif
#endif
#ifndef FS_H
#include <FS.h>
#endif
#ifndef ATTACHSPIFFS
#define ATTACHSPIFFS 0
#endif
#ifndef USESPIFFSEDITOR
#define USESPIFFSEDITOR 0
#endif
#if USESPIFFSEDITOR > 0
#ifndef SPIFFSEditor_H_
#include <SPIFFSEditor.h>
#endif
#endif
#ifndef SoftwareSerial_h
#include <SoftwareSerial.h>
#endif
//</editor-fold desc="includes">
//<editor-fold desc="external-defines">
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif
#if USEOTA > 0
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "esp32ota"
#endif
#ifndef OTA_PASSWORD
#define OTA_PASSWORD "21232f297a57a5a743894a0e4a801fc3"
#endif
#endif
#ifndef ADMIN_PASSWORD
#define ADMIN_PASSWORD "admin"
#endif
#ifndef WIFI_AP
#define WIFI_AP "esp32ap"
#endif
#ifndef SSID_NAME
#define SSID_NAME "myhotspot"
#endif
#ifndef SSID_PASSWORD
#define SSID_PASSWORD "esp32connect"
#endif
#ifndef DEFDELAY
#define DEFDELAY 250
#endif
#ifndef INITSPIFFS
#define INITSPIFFS 0
#endif
#ifndef DEBUG
#define DEBUG 1
#endif

#define SWSER_BAUD_RATE 38400
#define RXPIN 0
#define TXPIN 2
//</editor-fold desc="external-defines">
//<editor-fold desc="camera-defines">
#define DEFAULT_IMAGE_FILENAME "/latest.jpg"
#define CAMERABUFFSIZ 100
#define CAMERADELAY 10

#define VC0706_RESET  0x26
#define VC0706_GEN_VERSION 0x11
#define VC0706_READ_FBUF 0x32
#define VC0706_GET_FBUF_LEN 0x34
#define VC0706_FBUF_CTRL 0x36
#define VC0706_READ_DATA 0x30
#define VC0706_WRITE_DATA 0x31
#define VC0706_STOPCURRENTFRAME 0x0
#define VC0706_640x480 0x00
#define VC0706_320x240 0x11
#define VC0706_160x120 0x22

#define VC0706_DOWNSIZE_CTRL 0x54
#define VC0706_DOWNSIZE_STATUS 0x55
#define VC0706_SET_PORT 0x24
#define VC0706_COMM_MOTION_CTRL 0x37
#define VC0706_COMM_MOTION_STATUS 0x38
#define VC0706_COMM_MOTION_DETECTED 0x39
#define VC0706_MOTION_CTRL 0x42
#define VC0706_MOTION_STATUS 0x43
#define VC0706_TVOUT_CTRL 0x44
#define VC0706_OSD_ADD_CHAR 0x45
#define VC0706_STOPNEXTFRAME 0x1
#define VC0706_RESUMEFRAME 0x3
#define VC0706_STEPFRAME 0x2
#define VC0706_MOTIONCONTROL 0x0
#define VC0706_UARTMOTION 0x01
#define VC0706_ACTIVATEMOTION 0x01
#define VC0706_SET_ZOOM 0x52
#define VC0706_GET_ZOOM 0x53

//</editor-fold desc="camera-defines">
//<editor-fold desc="global-variables">
bool hasCamera  = false;
bool hasSpiffs  = false;
SoftwareSerial swSer(RXPIN, TXPIN, false, 256);
AsyncWebServer httpserver(80);

// camera variables
uint8_t  camerabuff[CAMERABUFFSIZ+1];
uint8_t  bufferLen = 0;
uint16_t frameptr  = 0;
uint8_t  serialNum = 0;
//</editor-fold desc="global-variables">

//<editor-fold desc="debug-code">
#if DEBUG > 0
void debug_message(String m) {
    Serial.println("DEBUG: " + m);
    delay(DEFDELAY);
}
bool getFileSize(const char *fn) {
    if(!hasSpiffs) return false;
    File file = SPIFFS.open(fn, "r");

    if(!file){
        Serial.println("Failed to open file for reading");
        return false;
    }

    Serial.print("File size: ");
    Serial.print(file.size(), DEC);
    Serial.println(" bytes");

    file.close();
    return true;
}
#endif
//</editor-fold desc="debug-code">
//<editor-fold desc="camera-methods">
#if DEBUG > 0
void camera_printBuff() {
    debug_message("camera_printBuff:");
    for (uint8_t i = 0; i< bufferLen; i++) {
        Serial.print(" 0x"+String(camerabuff[i], HEX));
    }
    Serial.println();
};
#endif
void camera_init(void) {
#if DEBUG > 1
    debug_message("camera_init: ");
#endif
    frameptr  = 0;
    bufferLen = 0;
    serialNum = 0;
};
void camera_sendCommand(uint8_t cmd, uint8_t *args = 0, uint8_t argn = 0) {
#if DEBUG > 1
    debug_message("camera_sendCommand: ");
#endif
    swSer.write((byte)0x56);
    swSer.write((byte)serialNum);
    swSer.write((byte)cmd);


    for (uint8_t i=0; i<argn; i++) {
        swSer.write((byte)args[i]);
#if DEBUG > 2
        Serial.print(" 0x"+String(args[i], HEX));
#endif
    }

#if DEBUG > 2
    Serial.println();
#endif
};
uint8_t camera_readResponse(uint8_t numbytes, uint8_t timeout) {
#if DEBUG > 1
    debug_message("camera_readResponse: ");
#endif
    uint8_t counter = 0;
    bufferLen = 0;

    while ((timeout != counter) && (bufferLen != numbytes)){
        if (swSer.available() <= 0) {
            delay(1);
            counter++;
            continue;
        }
        counter = 0;
        // there's a byte!
        camerabuff[bufferLen++] = swSer.read();
    }
#if DEBUG > 2
    camera_printBuff();
    Serial.print("bufferLen: ");
    Serial.println(bufferLen, DEC);
#endif
    return bufferLen;
};
boolean camera_verifyResponse(uint8_t command) {
#if DEBUG > 1
    debug_message("camera_verifyResponse: ");
#endif
    bool res = ((camerabuff[0] != 0x76) || (camerabuff[1] != serialNum) || (camerabuff[2] != command) || (camerabuff[3] != 0x0));
#if DEBUG > 2
    Serial.println(res?"invalid":"valid");
#endif
    return !res;
};
boolean camera_runCommand(uint8_t cmd, uint8_t *args, uint8_t argn, uint8_t resplen, boolean flushflag=true) {
#if DEBUG > 1
    debug_message("camera_runCommand: ");
#endif
    // flush out anything in the buffer?
    if (flushflag) camera_readResponse(100, 10);

    camera_sendCommand(cmd, args, argn);
    if (camera_readResponse(resplen, 200) != resplen) return false;
    if (! camera_verifyResponse(cmd)) return false;
    return true;
};
boolean camera_reset() {
#if DEBUG > 1
    debug_message("camera_reset: ");
#endif
    uint8_t args[] = {0x0};

    return camera_runCommand(VC0706_RESET, args, 1, 5);
};
uint8_t camera_getImageSize() {
#if DEBUG > 1
    debug_message("camera_getImageSize: ");
#endif
    uint8_t args[] = {0x4, 0x4, 0x1, 0x00, 0x19};
    if (! camera_runCommand(VC0706_READ_DATA, args, sizeof(args), 6))
        return -1;

    return camerabuff[5];
}
boolean camera_setImageSize(uint8_t x) {
#if DEBUG > 1
    debug_message("camera_setImageSize: ");
#endif
    uint8_t args[] = {0x05, 0x04, 0x01, 0x00, 0x19, x};

    return camera_runCommand(VC0706_WRITE_DATA, args, sizeof(args), 5);
}
boolean camera_begin(uint16_t baud) {
#if DEBUG > 1
    debug_message("camera_begin: ");
#endif
    swSer.begin(baud);
    return camera_reset();
};
boolean camera_FrameBuffCtrl(uint8_t command) {
#if DEBUG > 1
    debug_message("camera_FrameBuffCtrl: ");
#endif
    uint8_t args[] = {0x1, command};
    return camera_runCommand(VC0706_FBUF_CTRL, args, sizeof(args), 5);
}
boolean camera_takePicture() {
#if DEBUG > 1
    debug_message("camera_takePicture: ");
#endif
    frameptr = 0;
    return camera_FrameBuffCtrl(VC0706_STOPCURRENTFRAME);
}
uint32_t camera_frameLength(void) {
#if DEBUG > 1
    debug_message("camera_frameLength: ");
#endif
    uint8_t args[] = {0x01, 0x00};
    if (!camera_runCommand(VC0706_GET_FBUF_LEN, args, sizeof(args), 9))
        return 0;

    uint32_t len;
    len = camerabuff[5];
    len <<= 8;
    len |= camerabuff[6];
    len <<= 8;
    len |= camerabuff[7];
    len <<= 8;
    len |= camerabuff[8];

    return len;
}
/** original read picture **/
/*
bool camera_readPicture(uint8_t n) {
#if DEBUG > 1
    debug_message("camera_readPicture: ");
#endif
    uint8_t args[] = {0x0C, 0x0, 0x0A, 0, 0, frameptr >> 8, frameptr & 0xFF, 0, 0, 0, n, CAMERADELAY >> 8, CAMERADELAY & 0xFF};

    if (! camera_runCommand(VC0706_READ_FBUF, args, sizeof(args), 5, false)) return false;
    // read into the buffer PACKETLEN!
    if (camera_readResponse(n+5, CAMERADELAY) > 0) {
        frameptr += n;
        return true;
    } else {
        return false;
    }
}
 */
bool camera_readPictureToFile(const char* fn) {
    Serial.println("in readPictureToFile"); delay(500);
#if DEBUG > 1
    debug_message("camera_readPictureToFile: ");
#endif

    if(!hasCamera || !hasSpiffs) return false;

    // Get the size of the image (frame) taken
    uint16_t jpglen = camera_frameLength();
    // read 32/64 bytes at a time;
    uint16_t minByte = 32; // change 32 to 64 for a speedup but may not work with all setups!
#if DEBUG > 2
    int32_t time = millis();
#endif

    // Create an image with the name Image.jpg
    // remove file if it already exists
    if (!SPIFFS.exists(fn)) {
#if DEBUG > 1
        Serial.println("Image file already exists, removing first");
#endif
        SPIFFS.remove(fn);
    }
    // Open the file for writing
#if DEBUG > 0
    Serial.print("Writing file to SPIFFS: '");
    Serial.print(fn);
    Serial.print("' ");
    Serial.print(jpglen, DEC);
    Serial.println(" bytes");
#endif
    Serial.println("Opening file");
    File imgFile = SPIFFS.open(fn, "w");

    // Read all the data up to # bytes!
    while (jpglen > 0) {
        uint8_t bytesToRead = min(minByte, jpglen);
        uint8_t args[] = {0x0C, 0x0, 0x0A, 0, 0, frameptr >> 8, frameptr & 0xFF, 0, 0, 0, bytesToRead, CAMERADELAY >> 8, CAMERADELAY & 0xFF};

        if (! camera_runCommand(VC0706_READ_FBUF, args, sizeof(args), 5, false)) return false;
        // read into the buffer PACKETLEN!

        uint8_t numbytes = bytesToRead+5;
        uint8_t timeout = CAMERADELAY;
        uint8_t counter = 0;
        bufferLen = 0;

        while ((timeout != counter) && (bufferLen != numbytes)){
            if (swSer.available() <= 0) {
                delay(1);
                counter++;
                continue;
            }
            counter = 0;
            // there's a byte!
            uint8_t b = swSer.read();
            Serial.println("filewrite"); delay(100);
            imgFile.write((byte)b);
            camerabuff[bufferLen++] = b;
        }
        if(bufferLen == 0) break;

        frameptr += bytesToRead;
        jpglen -= bytesToRead;
    }

    imgFile.close();
#if DEBUG > 2
    Serial.println("done!");

    time = millis() - time;
    Serial.print(time);
    Serial.println(" ms elapsed");

    camera_printBuff();
    Serial.print("bufferLen: ");
    Serial.println(bufferLen, DEC);
#endif

    if(bufferLen > 0) {
        return true;
    }
    else return false;
}

#if DEBUG > 0
bool camera_getVersion(void) {
#if DEBUG > 1
    debug_message("camera_getVersion: ");
#endif
    if(!hasCamera) return false;

    uint8_t args[] = {0x01};

    camera_sendCommand(VC0706_GEN_VERSION, args, 1);

    // get reply
    bool res = camera_readResponse(CAMERABUFFSIZ, 200);
    if (!res) return false;

    camerabuff[bufferLen] = 0;  // end it!
    //return (char *)camerabuff;  // return it!
    return true;
}
bool getCameraInfo() {
#if DEBUG > 1
    debug_message("getCameraInfo: ");
#endif
    if(!hasCamera) return false;
    if(camera_getVersion()) {
        Serial.println("-----------------");
        Serial.print((char *) camerabuff);
        Serial.println("-----------------");
        return true;
    } else {
        Serial.println("Failed to get version");
        return false;
    }
}
#endif

/**
 * snap a picture on the jpeg camera using Adafruit_VC0706 library
 *   send resulting file direct from filesystem via ESP8266AsyncWebServer
 *
 * @param {AsyncWebServerRequest*} request
 * @return
 */
//</editor-fold desc="camera-methods">
//<editor-fold desc="httphandlers">
/**
 * empty https success handler
 *
 * @param {AsyncWebServerRequest} request
 */
void returnOK(AsyncWebServerRequest *request) { request->send(200, "text/plain", ""); }
/**
 * generic http 500 failure message
 *
 * @param {AsyncWebServerRequest} request
 * @param {String} msg
 */
void returnFail(AsyncWebServerRequest *request, String msg) {request->send(500, "text/plain", msg + "\r\n");}
/**
 * fallback handler for non-specified paths
 *   issues a 404 "Not Found"
 *
 * @param {AsyncWebServerRequest} request
 */
void handleNotFound(AsyncWebServerRequest *request) { request->send(404, "text/plain", "Not found"); }
/**
 * snap a picture on the jpeg camera using Adafruit_VC0706 library
 *   send resulting file direct from filesystem via ESP8266AsyncWebServer
 *
 * @param {AsyncWebServerRequest*} request
 * @return
 **/
void snapPic(AsyncWebServerRequest *request) {
    if(!hasCamera) {
        returnFail(request, "Camera was not initialized");
        return;
    }
    if(!hasSpiffs) {
        returnFail(request, "Filesystem was not initialized");
    }
#if DEBUG > 1
    Serial.println("Snap in 3 secs...");
    delay(3000);
#endif

    if (camera_readPictureToFile("/image.jpg")) {
#if DEBUG > 0
        Serial.println("Picture taken!");
        getFileSize("/image.jpg");
#endif
        request->send(SPIFFS, "/image.jpg", "image/jpeg");
        return;
    }
    else {
#if DEBUG > 0
        Serial.println("Failed to snap!");
#endif
        returnFail(request, "Failed to snap picture.");
        return;
    }
}
//</editor-fold>
//<editor-fold desc="initialization-methods">
/**
 * initialize wifi station and access point modes based on defined parameters
 *
 * @return {bool} returns true on success
 */
bool initWiFi() {
#if DEBUG > 0
    Serial.println("Connecting to WiFi...");
#endif
    WiFi.mode(WIFI_AP_STA);
#if DEBUG > 0
    Serial.print("Starting AP: ");
    Serial.println(WIFI_AP);
#endif
    WiFi.softAP(WIFI_AP, ADMIN_PASSWORD);
#if DEBUG > 0
    Serial.print("Connecting to network: ");
    Serial.println(SSID_NAME);
#endif
    WiFi.begin(SSID_NAME, SSID_PASSWORD);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
#if DEBUG > 0
        Serial.println("Connection Failed!");
#endif
        return false;
    }
#if DEBUG > 0
    Serial.println(WiFi.localIP());
#endif
    return true;
}
#if USEMDNS > 0
/**
 * initialize mDNS service and add http on port 80
 */
void initMDNS() {
#if DEBUG > 0
    Serial.println("initializing mDNS Server");
#endif
    MDNS.addService("http","tcp",80);
}
#endif
#if USEOTA > 0
/**
 * initialize over-the-air (OTA) update listener
 */
void initOTA() {
#if DEBUG > 0
    Serial.println("Initializing OTA listener...");
#endif
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    // ArduinoOTA.setPassword(ADMIN_PASSWORD);
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    ArduinoOTA.setPasswordHash(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
#if DEBUG > 0
        Serial.println("Start updating " + type);
#endif
    });
    ArduinoOTA.onEnd([]() {
#if DEBUG > 0
        Serial.println("\nEnd");
#endif
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
#if DEBUG > 0
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
#endif
    });
    ArduinoOTA.onError([](ota_error_t error) {
#if DEBUG > 0
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
#endif
    });

    ArduinoOTA.begin();
}
#endif
/**
 * initialize Asyncronous http server on port 80 complete with handler definitions
 */
void initHttp() {
#if DEBUG > 0
    Serial.println("initializing HTTP Server");
#endif
    // attach AsyncWebSocket
    //ws.onEvent(onEvent);
    //httpserver.addHandler(&ws);
    // attach AsyncEventSource
    //httpserver.addHandler(&events);
#if USESPIFFSEDITOR > 0
    httpserver.addHandler(new SPIFFSEditor("admin", ADMIN_PASSWORD, SPIFFS));
#endif
#if ATTACHSPIFFS > 0
    // attach filesystem root at URL /fs
    httpserver.serveStatic("/spiffs", SPIFFS, "/");
#endif

    httpserver.on("/snap", HTTP_GET, snapPic);
    httpserver.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.htm");
    });
    httpserver.onNotFound(handleNotFound);
    httpserver.begin();
#if DEBUG > 0
    Serial.println("HTTP Server started");
#endif
}
/**
 * initialize SPIFFS filesystem
 *
 * @return {bool} returns true on success
 */
bool initSPIFFS() {
#if DEBUG > 0
    Serial.println("Starting SPIFFS file system...");
#endif
    if(!SPIFFS.begin())
    {
#if DEBUG > 0
        Serial.println("Starting SPIFFS Failed!");
        delay(DEFDELAY*5);
#endif
        return false;
    }
#ifdef INITSPIFFS
#if INITSPIFFS > 1
    else
    {
        SPIFFS.format();
#if DEBUG > 0
        Serial.println("Formatting SPIFFS filesystem.");
#endif
    }
#endif
#endif

    return true;
}
/**
 * initialize the serial camera on the software serial conection
 *
 * @return true if successful
 */
bool initCamera() {
#if DEBUG > 0
    debug_message("Initializing camera");
#endif
    if(camera_begin(SWSER_BAUD_RATE)) {
        // Print additional information (optional)
#if DEBUG > 0
        Serial.println("Camera Found!");
        getCameraInfo();
#endif

        // Set the picture size - you can choose one of 640x480, 320x240 or 160x120
        // Remember that bigger pictures take longer to transmit!

        camera_setImageSize(VC0706_640x480);        // biggest
        //cam.setImageSize(VC0706_320x240);        // medium
        //cam.setImageSize(VC0706_160x120);          // small

#if DEBUG > 0
        // You can read the size back from the camera (optional, but maybe useful?)
        uint8_t imgsize = camera_getImageSize();
        Serial.print("Image size: ");
        if (imgsize == VC0706_640x480) Serial.println("640x480");
        if (imgsize == VC0706_320x240) Serial.println("320x240");
        if (imgsize == VC0706_160x120) Serial.println("160x120");
#endif
    } else {
#if DEBUG > 0
        Serial.println("Camera not found");
#endif
        return false;
    }
    return true;
}
#if DEBUG > 0
#endif

//</editor-fold desc="initialization-methods">

void setup() {
    delay(DEFDELAY * 5);
    Serial.begin(SERIAL_BAUD);         // Start the Serial communication to send messages to the computer
    delay(DEFDELAY);
#if DEBUG > 0
    debug_message("Serial monitoring started");
#endif
    hasSpiffs = initSPIFFS();
    delay(DEFDELAY);

    hasCamera = initCamera();
    delay(DEFDELAY);

    initWiFi();
    delay(DEFDELAY);

#if USEOTA > 0
    initOTA();
    delay(DEFDELAY);
#endif

    initHttp();
    delay(DEFDELAY);

#if USEMDNS > 0
    initMDNS();
    delay(DEFDELAY);
#endif

#if DEBUG > 0
    debug_message("Setup complete!");
#endif
}

void loop() {
    delay(10);
#if USEOTA > 0
    // handle any OTA requests
    ArduinoOTA.handle();
#endif
}
