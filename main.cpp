#include "mbed.h"
#include "ultrasonic.h"
#include "rtos.h"

 void dist(int distance)
{
    //put code here to execute when the distance has changed
    printf("Distance %d mm\r\n", distance);
}

ultrasonic mu(p6, p7, .1, 1, &dist);    //Set the trigger pin to D8 and the echo pin to D9
                                        //have updates every .1 seconds and a timeout after 1
                                        //second, and call dist when the distance changes
volatile int points = 0; 
volatile dist = -1; 
volatile boolean game = 0; 

void checkPoint()
{
   mtx.lock(); 
   if(game)
   {
       if(dist < 70 && dist > 0)
        {
             points++; 
             }
    mtx.unlock(); 
    while(1) { Thread::yield(500) };
       
   
}
    

int main()
{
    Thread pts; 
    
    pts.
    mu.startUpdates();//start measuring the distance
    while(1)
    {
        //Do something else here
        mtx.lock();
        dist = mu.checkDistance();     //call checkDistance() as much as possible, as this is where
        mtx.unlock();                     //the class checks if dist needs to be called.
    }
}
