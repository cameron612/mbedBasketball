#include "mbed.h"
#include <string> 
#include "ultrasonic.h"
#include "uLCD_4DGL.h"
#include "wave_player.h"

Serial pc(USBTX, USBRX);

// Network interface
NetworkInterface *net;

Thread threadLED, threadGame; //thread to update game and LED
Mutex mtx; //mutex lock
Timer t, delay;
//AnalogOut DACout(p18);    // Speaker
PwmOut speaker(p26);
//wave_player waver(&DACout);


PwmOut RGBLed_r(p21);
PwmOut RGBLed_b(p24);
PwmOut RGBLed_g(p25);

uLCD_4DGL uLCD(p28,p27,p30);
void dist(int distance);
ultrasonic mu(p9, p10, .1, 1, &dist); //call dist when distance changes

//Global thread variables and flags
volatile int points = 0;
volatile int prevPoints = -1;
volatile int game = 1; //game flag 
volatile int endGame = 0;
std::string url = "c926e5b7.ngrok.io";

//method to get sonar distance
void dist(int distance)
{
	delay.start();
    //printf("Game: %d", game);
    mtx.lock();
    if(distance < 70 && game == 1 && delay.read() > 1) { //range to be within and add point
    	delay.reset();
        prevPoints = points;
        ++points; //incrememnt points 
        speaker.period(1.0/550.0); // 500hz period
	    speaker =0.1; //50% duty cycle - max volume
	    wait(0.25);
	    speaker=0.0;
	    speaker.period(1.0/850.0); // 500hz period
	    speaker =0.1; //50% duty cycle - max volume
	    wait(0.25);
	    speaker=0.0;
    }

    mtx.unlock();

}

/*
void rgbLED(void) - thread function
Inputs: Uses global Game flag to set the correct color to indicate if 
the game is happening or not
Outputs: LED color, green for game, purple for game over
*/
void rgbLED(void) 
{
    while(1) {
        //printf("In RGB\n");
        mtx.lock(); //mtx lock for global variables
        if(game == 1) {
            //printf("In if");
            RGBLed_r = 0; //set color to green for game start
            RGBLed_g = 255;
            RGBLed_b = 0;
        } else {
            //printf("Not in If\n");
            RGBLed_r = 200; //set color to purple for game over
            RGBLed_g = 0;
            RGBLed_b = 255;
        }
        //game = !game;
        mtx.unlock();
        Thread::wait(500);
    }
}

/*
void updateScore(void) - thread function
Inputs: Global game flag, int prevPoints, int points
Outputs: Calls distance sensor function if the game is active (1)
  Adjusts the uLCD screen to display the number of points. 
  Timer checks for if 30 seconds have passed to end the game (game = 0). 
  When the game ends, prints out Game over and score. 
*/
void updateScore(void)
{
    while(1) {
        mtx.lock();
        if(game == 1) {
            t.start();
            mu.checkDistance();
            //wait(100); 
            if(prevPoints != points) {
                uLCD.locate(0,0);
                uLCD.text_width(5);
                uLCD.text_height(5);
                uLCD.printf("%d", points);

            }
            if(t.read() > 30) {
                game = 0;
                uLCD.locate(0,0);
                uLCD.text_width(3); 
                uLCD.text_height(3); 
                uLCD.printf("Game\n over.\nScore: %d", points);
                t.stop();
                t.reset();
            }
            //printf("Game: %d, Time: %f \n", game, t.read()); 
        }
        mtx.unlock();
        //Thread::wait(500);
    }
}

// Socket + wait for game trigger
int main() {
    //run repeatedly
	while(1) {
    	
    int remaining;
    int rcount;
    char *p;
    char *buffer = new char[256];
    nsapi_size_or_error_t result;

    // Bring up the ethernet interface
    //printf("Mbed OS Socket via ethernet\n");

#ifdef MBED_MAJOR_VERSION
    printf("Mbed OS version: %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
#endif
	
    net = NetworkInterface::get_default_instance();
	
    if (!net) {
        printf("Error! No network inteface found.\n");
        return 0;
    }

    result = net->connect();
    if (result != 0) {
        printf("Error! net->connect() returned: %d\n", result);
        return result;
    }

    // Show the network address
    //printf("before hang\n");
    const char *ip = net->get_ip_address();
    const char *netmask = net->get_netmask();
    const char *gateway = net->get_gateway();
    //printf("after hang\n");
    printf("IP address: %s\n", ip ? ip : "None");
    printf("Netmask: %s\n", netmask ? netmask : "None");
    printf("Gateway: %s\n", gateway ? gateway : "None");

    // Open a socket on the network interface, and create a TCP connection to ifconfig.io
    TCPSocket socket;
    
    
    // Send a simple http request
    std::string strbuffer;
    const char * sbuffer;
    if (endGame) {
    	char s[21];
    	sprintf(s, "%d", points);
    	std::string temp1 = s;
    	char x[21];
    	sprintf(x, "%d", strlen(s));
    	std::string temp2 = x;
		//sprintf(sbuffer, "POST /game?score=%d HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", points, url, strlen(s));
    	strbuffer = "POST /game?score=" + temp1 + " HTTP/1.1\r\nHost: " + url + "\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: "+ temp2 +"\r\nConnection: close\r\n\r\n";
    } else {
    	strbuffer = "GET / HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";
    }
    sbuffer = strbuffer.c_str();
    nsapi_size_t size = strlen(sbuffer);

    result = socket.open(net);
    if (result != 0) {
        printf("Error! socket.open() returned: %d\n", result);
    }

    result = socket.connect(url.c_str(), 80);
    if (result != 0) {
        printf("Error! socket.connect() returned: %d\n", result);
        goto DISCONNECT;
    }
    
    // Loop until whole request sent
    while(size) {
        result = socket.send(sbuffer+result, size);
        if (result < 0) {
            printf("Error! socket.send() returned: %d\n", result);
            goto DISCONNECT;
        }
        size -= result;
        printf("sent %d [%.*s]\n", result, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);
    }
	if (endGame) break;
    // Receieve an HTTP response and print out the response line
    remaining = 256;
    rcount = 0;
    p = buffer;
    while (remaining > 0 && 0 < (result = socket.recv(p, remaining))) {
        p += result;
        rcount += result;
        remaining -= result;
    }
    if (result < 0) {
        printf("Error! socket.recv() returned: %d\n", result);
        goto DISCONNECT;
    }
	// the HTTP response code
    printf("recv %d [%.*s]\n", rcount, strstr(buffer, "\r\n")-buffer, buffer);
	int resp_body = buffer[199] - '0';
	printf("\nResponse Body: %d\n", resp_body);
	//start game if value was 1
	if (resp_body == 1) {
		mu.startUpdates();
    
    	threadGame.start(updateScore); //start updating the score
    	threadLED.start(rgbLED); //start checking LED lights
    	wait(30); //wati for game to run
	    //strbuffer = "POST /game?score=11 HTTP/1.1\r\nHost: " + url + "\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
		endGame = 1;
    	//break;
    	
	}
    delete[] buffer;

DISCONNECT:
    // Close the socket to return its memory and bring down the network interface
    socket.close();

    // Bring down the ethernet interface
    net->disconnect();
    printf("Done\n");
    wait(5.0);
    }
    wait(5.0);
    //send score before restarting
    //sendScore(points);
    //restart system
    NVIC_SystemReset();
}