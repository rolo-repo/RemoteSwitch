#define ENABLE_LOGGER
#include "SerialOutput.h"
#include "Constants.h"
#include "Led.h"
#include "Switch.h"
#include "Button.h"

#include "resource.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "EEPROM_Adapter.h"
#include <TimeManager.h>

#include <BlynkSimpleEsp8266.h>
#include <heltec.h>

#if defined ARDUINO_ESP8266_NODEMCU || defined ARDUINO_wifi_kit_8
#define ENB_PIN D3
#define STS_LED_PIN D7
#define SWTCH_PIN D6
#define RST_BTN_PIN D8
#endif

String ssid;
String ssid_password;
String blynk_auth;
String blynk_domain;
uint16 blynk_port;

void swchOn();
void swchOff();


Led stsLed(STS_LED_PIN);
Switch_t swch( SWTCH_PIN, &swchOff , &swchOn );
arduino::utils::Timer idleTimer("[IDLE]");
arduino::utils::Timer displayTimer("[Display]");

ESP8266WebServer  server(80);

WidgetLED ackLed(V3);

void swchOn()
{
	LOG_MSG("Switch ON");
	digitalWrite(ENB_PIN, HIGH);
	ackLed.on();
	//stsLed.turn_off();
}

void swchOff()
{
	LOG_MSG("Switch OFF");
	digitalWrite(ENB_PIN, LOW);
	ackLed.off();
	//stsLed.turn_on();
}

class SetUpWiFi_RequestHandler : public RequestHandler
{

public:

	bool canHandle(HTTPMethod method, String uri) {
		if (uri.startsWith("/wifi") && method == HTTP_GET)
			return true;
		else
			return false;
	}

	bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) {
		const char* INPUT_1 = "ssid";
		const char* INPUT_2 = "ssidp";

		const char* INPUT_3 = "blynk_auth";
		const char* INPUT_4 = "blynk_domain";
		const char* INPUT_5 = "blynk_port";


		// GET input1 value on <ESP_IP>/get?input1=<inputMessage>
		if (server.hasArg(INPUT_1)) {
			ssid = server.arg(INPUT_1);
			ssid.trim();
		}

		if (server.hasArg(INPUT_2) && server.arg(INPUT_2).length() > 0) {
			ssid_password = server.arg(INPUT_2);
			ssid_password.trim();
		}


		if (server.hasArg(INPUT_3) && server.arg(INPUT_3).length() > 0) {
			blynk_auth = server.arg(INPUT_3);
			blynk_auth.trim();
		}


		if (server.hasArg(INPUT_4) && server.arg(INPUT_4).length() > 0) {
			blynk_domain = server.arg(INPUT_4);
			blynk_domain.trim();
		}

		if (server.hasArg(INPUT_5) && server.arg(INPUT_5).length() > 0) {
			blynk_port = (uint16)server.arg(INPUT_5).toInt();
		}

		//LOG_MSG("Saving data ssid " << ssid << " pass " << ssid_password << " blynk auth" << blynk_auth);

		uint16 index = 0;
		index = EEPROM_Adapter_t::save(ssid);
		index = EEPROM_Adapter_t::save(ssid_password, index);
		index = EEPROM_Adapter_t::save(blynk_auth, index);
		index = EEPROM_Adapter_t::save(blynk_domain, index);
		index = EEPROM_Adapter_t::save(blynk_port, index);

		server.send(200, "text/html", success_html);

		//restart in 5 sec
	 	idleTimer.addTask( TIME.getEpochTime() + 5, [](long&) { RESET(); });
	}
};

void notFound() {
	String message = "Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";
	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
	server.send(404, "text/plain", message);
}

void display(String message, short line = 0 , bool display_on = false)
{
	using namespace arduino::utils;
	LOG_MSG(message);

	if ( display_on )
	{
		Heltec.display->displayOn();
		displayTimer = Timer("[Display]");
		displayTimer.addTask( TIME.getEpochTime() + Timer::HOUR , [](long&) { Heltec.display->displayOff(); });
	}

	if (0 == line)
		Heltec.display->clear();

	//Heltec.display->setFont(ArialMT_Plain_10);
	Heltec.display->drawString(0, line * 13, message);
	Heltec.display->display();
}

BLYNK_WRITE(V0)
{
	(0 == param.asInt()) ? swchOff() : swchOn();
}

// the setup function runs once when you press reset or power the board
void setup() {
	LOG_MSG_BEGIN(115200);
	pinMode(ENB_PIN, OUTPUT);
	pinMode(STS_LED_PIN, OUTPUT);
	pinMode(SWTCH_PIN, INPUT_PULLUP);
	
	stsLed.turn_off();
	server.begin();
	//The bellow are must to prevent failure on WDT
	ESP.wdtDisable();
	ESP.wdtEnable(WDTO_8S);

	WiFi.persistent(false);
	WiFi.disconnect();

	String _HOST("WebSWCH_");
	_HOST += arduino::utils::Constants::ID();

	{
		Heltec.begin(true /*DisplayEnable Enable*/, false /*Serial Enable*/);
		Heltec.display->init();
		Heltec.display->flipScreenVertically();
		Heltec.display->clear();
		Heltec.display->setFont(ArialMT_Plain_10);
	}
	
	EEPROM_Adapter_t::begin(512);

	bool sts = true;
	uint16 index = 0;
	sts &= EEPROM_Adapter_t::load(ssid, index);
	sts &= EEPROM_Adapter_t::load(ssid_password, index);
	sts &= EEPROM_Adapter_t::load(blynk_auth, index);
	sts &= EEPROM_Adapter_t::load(blynk_domain, index);
	sts &= EEPROM_Adapter_t::load(blynk_port, index);
	if (!sts)
	{
		ssid = "";
		ssid_password = "";
		blynk_auth = "";

		LOG_MSG("Setting soft-AP ... ");

		IPAddress local_IP(192, 168, 2, 1);
		IPAddress gateway(192, 168, 2, 1);
		IPAddress subnet(255, 255, 255, 0);
		
		if (WiFi.softAPConfig(local_IP, gateway, subnet) && WiFi.softAP(_HOST.c_str()))
		{
			LOG_MSG("Soft-AP IP address = " << WiFi.softAPIP().toString() << " (" << _HOST << ")");
			display(_HOST);
			display("IP:" + WiFi.softAPIP().toString() , 1 ,true);
		}
		else
		{
			LOG_MSG("Critical error!!! , not able to start server");
			RESET();
		}
		
		server.on("/", []() { server.send(200, "text/html", setupWiFiHTML); });
		server.addHandler(new SetUpWiFi_RequestHandler());

		idleTimer.addRecuringTask(TIME.getEpochTime(), 5, [](long&) {stsLed.fade(500); });
	}
	else
	{
		LOG_MSG(   "Loaded data ssid "	<< ssid 
				<< " pass "				<< ssid_password 
				<< " blynk auth "		<< blynk_auth 
				<< " blynk domain "		<< blynk_domain 
				<< " blynk port "		<< blynk_port);

		LOG_MSG("Going to connect to " << ssid << " host name " << _HOST);
		
		WiFi.mode(WIFI_STA);
		WiFi.begin(ssid.c_str(), ssid_password.c_str());
		WiFi.setAutoReconnect(true);
		WiFi.hostname(_HOST);
	
	   	Blynk.config(blynk_auth.c_str(), blynk_domain.c_str(), blynk_port);

// 		static WiFiEventHandler e1, e2, e3;
// 		static IPAddress myIP;
// 		e1 = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {  stsLed.rapid_blynk(100);/* DISPLAY_ON = true;*//*LOG_MSG("Got IP " << event.ip.toString());*/ });// As soon WiFi is connected, start NTP Client
//		e2 = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) { stsLed.rapid_blynk(100); /* LOG_MSG("Lost connection" );*/ });
//	  	e3 = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event) {   stsLed.rapid_blynk(100);/* DISPLAY_ON = true;*//*LOG_MSG("Connected to network");*/ });
		
		idleTimer.addRecuringTask(TIME.getEpochTime(), 3, [](long& io_time) {   if (!WiFi.isConnected()) { stsLed.rapid_blynk(500); display(WiFi.SSID()); display("No WiFi found", 1, true); } else { display(WiFi.localIP().toString()); }  });
		idleTimer.addRecuringTask(TIME.getEpochTime(), 5, [](long& io_time) {   if (!Blynk.connected())  { stsLed.rapid_blynk(500); display(WiFi.SSID()); display("no connection to Blynk", 1 , true); } });

		server.on("/", []() { server.send(200, "text/html", on_off_html);  });
	}

	server.on("/on",  []() { stsLed.blynk(); swchOn(); server.send(200, "text/html", success_html);  });
	server.on("/off", []() { stsLed.blynk(); swchOff(); server.send(200, "text/html", success_html);  });
	
	idleTimer.addRecuringTask(TIME.getEpochTime(), 1, [](long& io_time) { (swch.getState()) ? stsLed.turn_on() : stsLed.turn_off(); });
}


// the loop function runs over and over again until power down or reset
void loop() {
	swch.run();
	idleTimer.run();
	TIME.run();
	displayTimer.run();

	static Button_t button(RST_BTN_PIN, []() {},
		[]() {},
		[]() {
		display("Cleaning eeprom");
		EEPROM_Adapter_t::clean();
		stsLed.rapid_blynk(500);
		RESET();
	});

	server.handleClient();
	button.run();
	Blynk.run();
}
