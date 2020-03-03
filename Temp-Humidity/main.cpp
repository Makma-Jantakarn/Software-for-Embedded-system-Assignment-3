#include <mbed.h>
#include "HTS221Sensor.h"
#define PRODUCTION
   
DigitalOut led(LED1);
InterruptIn button(USER_BUTTON);
Thread t;
EventQueue queue(5 * EVENTS_EVENT_SIZE);
Serial pc(USBTX, USBRX);
WiFiInterface *wifi;

DevI2C i2c(PB_11, PB_10);
HTS221Sensor hts221(&i2c);

#if defined DEBUG
#include "http_request.h"
char* endpoint = "http://192.168.43.231:5000/update";
#else
#include "https_request.h"
char* endpoint = "https://aoomami1.herokuapp.com/update";

const char SSL_CA_PEM[] ="-----BEGIN CERTIFICATE-----\n"
"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
"+OkuE6N36B9K\n"
"-----END CERTIFICATE-----\n";
 #endif    

float humidity;
float temperature;

void pressed_handler() {

    pc.printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
	
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        pc.printf("\nConnection error: %d\n", ret);
        return;
    }

    pc.printf("Success\n\n");
    printf("MAC: %s\n", wifi->get_mac_address());
    printf("IP: %s\n", wifi->get_ip_address());
    printf("Netmask: %s\n", wifi->get_netmask());
    printf("Gateway: %s\n", wifi->get_gateway());
    printf("RSSI: %d\n\n", wifi->get_rssi());   

    while(1) {

        hts221.enable();
        hts221.get_humidity(&humidity);
        hts221.get_temperature(&temperature);
        hts221.disable();
        hts221.reset();

        char body[128];
        sprintf(body,"{\"humidity\":\%.2f,\"temperature\":\%.2f}", humidity, temperature);
#if defined DEBUG
        pc.printf("[DEBUG] ");
        HttpRequest* request = new HttpRequest(wifi, HTTP_POST, endpoint);
#else
        HttpsRequest* request = new HttpsRequest(wifi, SSL_CA_PEM, HTTP_POST, endpoint);
#endif
        request->set_header("Content-Type", "application/json");
        HttpResponse* response = request->send(body, strlen(body));
        pc.printf("Humidity: %f\t Temperature: %f\t Status: %d\n\r", humidity, temperature, response->get_status_code());
        //printf("Status is %d - %s\n", response->get_status_code(), response->get_status_message());
        //printf("body is:\n%s\n", response->get_body());
        
        delete request;// also clears out the response

        //wifi->disconnect();
        pc.printf("\nDone\n");    

        ThisThread::sleep_for(1000*60);
   }
   
}

int main() {

#if defined DEBUG
    pc.printf("[DEBUG]\n\r");
#endif

    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }
    t.start(callback(&queue, &EventQueue::dispatch_forever));
    button.fall(queue.event(pressed_handler));
    pc.printf("Ready\n");

    if (hts221.init(NULL)!=0) {
        printf("Could not initializ HTS211\n\r");
        return -1;
    }

    while(1) {
        led = !led;
        ThisThread::sleep_for(500);
    }

}