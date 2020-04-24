#include "mbed.h"
#include "rtos.h"
#include "ultrasonic.h"
#include "uLCD_4DGL.h"

Thread threadLED, threadGame; //thread to update game and LED
Mutex mtx; //mutex lock
Timer t;


PwmOut RGBLed_r(p21);
PwmOut RGBLed_b(p24);
PwmOut RGBLed_g(p25);

uLCD_4DGL uLCD(p9,p10,p11);

//Global thread variables and flags
volatile int points = 0;
volatile int prevPoints = -1;
volatile int game = 1; //game flag 
volatile int gamePrev = 0; //previous game flag. 

void dist(int distance)
{

    //printf("Game: %d", game);
    mtx.lock();
    if(distance < 70 && game == 1) { //range to be within and add point
        prevPoints = points;
        ++points; //incrememnt points 
        //wait(100); //oof don't wait it'll break
    }

    mtx.unlock();

}

ultrasonic mu(p6, p7, .1, 1, &dist); //call dist when distance changes

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
            }
            printf("Game: %d, Time: %f \n", game, t.read()); 
        }
        mtx.unlock();
        //Thread::wait(500);
    }
}

/*
int main()
Starts up the sensor for distance checking for goals. Begins
the threads to update the score/game and to change the light color. 
*/
int main()
{
    //uLCD.background_color(BLUE);
    //uLCD.printf("are you wired correctly?");
    mu.startUpdates();
    
    threadGame.start(updateScore); //start updating the score
    threadLED.start(rgbLED); //start checking LED lights

    while(1) {
        
        /*
        Use to set game flag to one?
        */ 
    }
}


