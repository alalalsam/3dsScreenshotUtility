## 3dsScreenshotUtility 
is an add-on to Luma3DS where, if your 3d mode slider is at maximum, your 3ds will take a screenshot of the top screen 3d image-pairs every 3.5 seconds and save it to luma/screenshots in your SD root. I made this so that I could collect a large dataset of 3d image-pairs, for training a machine learning model to predict 3d image-pairs from a single input image.

The 3ds 3d mode (and other stereoscopic 3d technology) works by displaying 2 images on the top screen at the same time, using a hardware shutter to block your view so that your eyes are viewing different images. The 2 images differ in that one is perspective shifted from the other, which creates an optical illusion that simulates depth to the viewer. A more in-depth explanation can be found [here](https://gbatemp.net/threads/better-stereoscopic-3d-patches-cheat-codes-releases-development-and-discussion.625945/). If you can easily generate these 3d image pairs, you can display 2d-images in 3d, with little effort. This could be useful for artists who have to manually "draw in" the 3d mode for movies, or for anyone who might want to implement 3d-viewing for any application.

## How it Works
My code instantiates 2 threads in the rosalina process. One thread screen-captures the top screen, sending it to cache, then calls the second thread. The second thread sleeps until it's called by the first, then it writes the cached screencap to your SD card. This is implemented in this way because if I tried to cache and write on the same thread, it would interrupt gameplay or screencap the image pairs too late, depending on which CPU core I used. 

The cache thread is housed on the SYSCORE, so I can freeze the kernel while caching to eliminate any chance of accidentally grabbing different frames for my image pairs. The write thread is housed on CORE3, so that it can write to SD in the backround, without interrupting the running application or system. 


## THIS CODE DOES NOT WORK ON OLD 3DS because I used CORE3, which is an extra CPU core only present on the new 3ds.


huge thanks to the creators of [Luma3DS](https://github.com/LumaTeam/Luma3DS), whose work comprises the majority of this repository. The only real addition I made was datasetCapture.c and .h files in sysmodules/rosalina/includes and source. 


TODO:
#**TO USE:** assuming you have modded your 3ds according to [this](https://3ds.hacks.guide/) guide, simply go to releases (on the right side of the screen) and place the boot.firm in your root 3ds directory, replacing the one thats currently there. It's probably bad practice 
i havenet added release yet, i gotta update and build with latest Luma3DS version first.
