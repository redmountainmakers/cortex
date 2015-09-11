int led1 = D6; // connected to solid-state relay
int led2 = D7; // the little blue LED built into the board
int buttonLow = D2;
int buttonInput = D1;
int buttonHigh = D0;

int ledState = HIGH;

TCPServer server = TCPServer(80);
char addr[16];

const int MAX_HTTP_LENGTH = 255;

char httpRequestBuffer[MAX_HTTP_LENGTH + 1];
int httpRequestLength;

char httpResponseBuffer[MAX_HTTP_LENGTH + 1];

void setup() {
    pinMode(buttonLow, OUTPUT);
    digitalWrite(buttonLow, LOW);
    
    pinMode(buttonInput, INPUT);
    
    // Writing an INPUT mode pin HIGH does not seem to work here,
    // like I hear it does on Arduino
    // so we're using an extra pin
    pinMode(buttonHigh, OUTPUT);
    digitalWrite(buttonHigh, HIGH);

    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    setState(HIGH);
    
    IPAddress localIP = WiFi.localIP();
    sprintf(addr, "%u.%u.%u.%u", localIP[0], localIP[1], localIP[2], localIP[3]);
    Spark.variable("Address", addr, STRING);
    Spark.variable("Request", httpRequestBuffer, STRING);
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
        setState(ledState == HIGH ? LOW : HIGH);
        while (digitalRead(buttonInput) == LOW) {
            delay(100);
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
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"POST here instead to toggle\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else if (httpRequestIs("POST /on")) {
        // Turn the lights on
        
        setState(HIGH);
        sprintf(
            httpResponseBuffer,
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"Lights turned on\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else if (httpRequestIs("POST /off")) {
        // Turn the lights off
        
        setState(LOW);
        sprintf(
            httpResponseBuffer,
            "{\"status\":\"ok\",\"lights\":\"%s\",\"message\":\"Lights turned off\"}",
            ledState == HIGH ? "on" : "off"
        );
        sendHttpResponse(client, httpResponseBuffer);
        
    } else if (httpRequestIs("POST /toggle")) {
        // Toggle lights
        
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
    client.println("Connection: close");
    client.println();
    client.println(json);
    client.flush();
    client.stop();
}