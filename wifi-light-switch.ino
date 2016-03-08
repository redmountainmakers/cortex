int led1 = D6; // connected to solid-state relay
int led2 = D7; // the little blue LED built into the board
int buttonLow = D4;
int buttonInput = D5;

int ledState = HIGH;

TCPServer server = TCPServer(80);
char addr[16];

int countButtonPress = 0;
int countButtonShort = 0;
int countHttpOn      = 0;
int countHttpOff     = 0;
int countHttpToggle  = 0;
int buttonDelay      = 0;

const int MAX_HTTP_LENGTH = 255;

char httpRequestBuffer[MAX_HTTP_LENGTH + 1];
int httpRequestLength;

char httpResponseBuffer[MAX_HTTP_LENGTH + 1];

void setup() {
    pinMode(buttonLow, OUTPUT);
    digitalWrite(buttonLow, LOW);
    
    pinMode(buttonInput, INPUT_PULLUP);
    
    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    setState(HIGH);
    
    IPAddress localIP = WiFi.localIP();
    sprintf(addr, "%u.%u.%u.%u", localIP[0], localIP[1], localIP[2], localIP[3]);
    Spark.variable("Address", addr, STRING);
    Spark.variable("Request", httpRequestBuffer, STRING);
    
    Particle.variable("cButtonPress", &countButtonPress, INT);
    Particle.variable("cButtonShort", &countButtonShort, INT);
    Particle.variable("cHttpOn"     , &countHttpOn     , INT);
    Particle.variable("cHttpOff"    , &countHttpOff    , INT);
    Particle.variable("cHttpToggle" , &countHttpToggle , INT);
    Particle.variable("buttonDelay" , &buttonDelay     , INT);
    
    server.begin();
}

void setState(const int state) {
    digitalWrite(led1, state);
    digitalWrite(led2, state);
    ledState = state;
}

void loop() {
    delay(100);
    
    if (digitalRead(buttonInput) == LOW) {
        int start = millis();
        bool registeredButtonPress = false;
        while (digitalRead(buttonInput) == LOW) {
            delay(10);
            buttonDelay = millis() - start;
            if (buttonDelay >= 50 && !registeredButtonPress) {
                countButtonPress++;
                registeredButtonPress = true;
                setState(ledState == HIGH ? LOW : HIGH);
            }
        }
        if (!registeredButtonPress) {
            countButtonShort++;
        }
    }
    
    TCPClient client = server.available();
    if (client.connected()) {
        processHttpRequest(client);
    }
}

void processHttpRequest(TCPClient client) {
    httpRequestLength = 0;
    bool foundNewLine = false;
    while (client.available()) {
        char c = client.read();
        if (c == '\r' || c == '\n') {
            foundNewLine = true;
        } else if (!foundNewLine && httpRequestLength < MAX_HTTP_LENGTH) {
            httpRequestBuffer[httpRequestLength++] = c;
        }
    }
    
    char * protocolPos = strstr(httpRequestBuffer, " HTTP/1.");
    if (protocolPos == NULL) {
        sendHttpResponse(client, "{\"status\":\"error\",\"message\":\"Malformed request\"}");
        return;
    }
    
    httpRequestLength = protocolPos - httpRequestBuffer;
    httpRequestBuffer[httpRequestLength] = '\0';
    
    if (httpRequestIs("GET /")) {
        // Status report + further instructions
        
        sprintf(
            httpResponseBuffer,
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"POST /on|/off|/toggle\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else if (httpRequestIs("POST /on")) {
        // Turn the lights on
        
        countHttpOn++;
        setState(HIGH);
        sprintf(
            httpResponseBuffer,
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"Lights turned on\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else if (httpRequestIs("POST /off")) {
        // Turn the lights off
        
        countHttpOff++;
        setState(LOW);
        sprintf(
            httpResponseBuffer,
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"Lights turned off\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else if (httpRequestIs("POST /toggle")) {
        // Toggle lights
        
        countHttpToggle++;
        setState(ledState == HIGH ? LOW : HIGH);
        sprintf(
            httpResponseBuffer,
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"Lights toggled\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else {
        // Unrecognized request
        
        sendHttpResponse(client, "{\"status\":\"error\",\"message\":\"Invalid request\"}");
        
    }
}

bool httpRequestIs(const char * methodAndPath) {
    return (strcmp(httpRequestBuffer, methodAndPath) == 0);
}

void sendHttpResponse(TCPClient client, const char * json) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: http://www.redmountainmakers.org");
    client.println("Cache-Control: no-cache");
    client.println("Connection: close");
    client.println();
    client.println(json);
    client.flush();
    client.stop();
}
