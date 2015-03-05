andromon-linux
==============
ANDROMON driver for android device


AOA protocol


This is a simple implementation for testing purpose. I just wanted to transfer an image over USB using AOA protocol. Corresponding android app code is available at "https://github.com/azizulhakim/andromon-android"


HOW TO TEST:

As it is only for testing purpose, the testing method is cumbersome. Follow the steps below for testing

1. Download the driver source
2. Run 'make'
3. Install the driver using "sudo insmod andromon.ko"
4. Install the Android app. Source available at "https://github.com/azizulhakim/andromon-android"
5. Connect android device using USB. Run the app, click on "Connect" button. You'll be asked for permission.
6. Inside the driver source tree, go to the Test folder. Run "gcc Transfer_Image.c". Then run "./a.out".
7. On android device, click on "Get Picture" button. The image file will be transferred and showed on the android app.
