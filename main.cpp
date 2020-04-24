#include "mbed.h"
#include "rtos.h"
#include "ultrasonic.h"
#include "uLCD_4DGL.h"

Thread threadLCD, threadLED, threadGame;
Mutex mtx;


PwmOut RGBLed_r(p21);
PwmOut RGBLed_b(p24);
PwmOut RGBLed_g(p25);

uLCD_4DGL uLCD(p9,p10,p11);

//Global thread variables
volatile int points = 0;
volatile int prevPoints = -1;
volatile int game = 0;

void dist(int distance)
{
    //code to execute when distance has changed

    printf("Game: %d", game); 
    if(distance < 70 && game) {
        prevPoints = points;
        ++points;
        // wait(100);
    }



    printf("Points: %d \r\n", points);

    printf("Distance %d mm\r\n", distance);

}

ultrasonic mu(p6, p7, .1, 1, &dist); //call dist when distance changes


void rgbLED(void)
{
    while(1) {
        printf("In RGB");
        mtx.lock();
        if(game) {
            printf("In if");
            RGBLed_r = 32;
            RGBLed_g = 255;
            RGBLed_b = 134;
        } 
        else 
        {
            printf("Not in If");
            RGBLed_r = 200;
            RGBLed_g = 0;
            RGBLed_b = 255;
        }
        game = !game;
        mtx.unlock();
        Thread::wait(500);
    }
}

void endGame()
{
    while(1) {
        mtx.lock();
        if(game) {
            printf("waiting to exit");
            wait(1000);
            game = 0;
            printf("Wait ended");
            uLCD.locate(2,2);
            uLCD.printf("Game over. Score: %d", points);
            break;

        }
        mtx.lock();
    }
}

void updateScore(void)
{
    while(1) {
        mtx.lock();
        if(game) {
            if(prevPoints != points) {
                uLCD.locate(0,0);
                uLCD.text_width(5);
                uLCD.text_height(5);
                uLCD.printf("%d", points);

            }

        }
        mtx.unlock();
        //Thread::wait(500);

    }
}




int main()
{
    //uLCD.background_color(BLUE);
    //uLCD.printf("are you wired correctly?");
    mu.startUpdates();
    //game = 0;
    //threadLED.start();
    threadLCD.start(updateScore);
    threadLED.start(rgbLED);
    //threadGame.start(endGame);

    while(1) {

        mu.checkDistance();

    }
}
//thread.start(rgbLED);


