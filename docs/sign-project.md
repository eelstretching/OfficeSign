I'm working on a project for a large, indoor LED sign using my favorite microcontroller the Raspberry Pi Pico 2W, which has support for Bluetooth LE. 

I would like to set things up so that the sign is hanging on the wall and it communicates with my laptop, a MacBook Pro with an M1 Max processor via Bluetooth.

# Pico Software

On CPU core 0, the Pico will be running the LED rendering code that I've been writing for the past two years. That code is currently in the PicoLEDs project. We can add a new subdirectory to the examples directory called "OfficeSign" that we'll use for the code for this project.

On CPU core 1, the Pico will be running the code to receieve messages from the laptop computer over Bluetooth. The Pico uses the BTStack library to manage Bluetooth communication, so you can write code to that API and it should work. 

When the Pico starts running, it should set itself up so that it can be paired with the laptop. There will be an application on the laptop that will communicate with the Pico and send it a variety of messages that will tell the Pico to take specific actions.

## Messages

### Set the time message

This message will provide the current Unix time from the laptop to the Pico, so that it can set its internal clock. 

### Meeting mode message

This message will provide information about the current meeting that is about to start. This information will include:

* The duration of the meeting, in minutes
* Who the meeting is with
* What the other meeting participant's time zone is
* Whether this is a "serious" or "fun" meeting

### Weather message

This message will provide information about the current weather to the Pico. This information will include:

* The current conditions: sunny, cloudy, partly cloudy, rain, snow
* The current temperature
* The expected high temperature
* The expected low temperature

### Clock mode message

This message will tell the Pico to display the current date and time, along with other information, like the weather if it has the current data.

### Vibe mode message

This message will tell the Pico to go into vibe mode, where it will display a variety of animations to make a display that is pleasing to the eye.

### Go dark message

This message will tell the Pico to turn off all the lights until a further message is received.

## Communication

The core that is managing the Bluetooth communication should communicate with the core running the animations for the sign via a shared buffer that is managed with a mutex. You can write the code for both ends of this communication. Remeber that we have to somehow alert the animation core that there's data available to be read (i.e., that a message has been received). You could use the pico multicore fifos for this notification.

You should use good C++ programming style and define classes for these messages and the communication as you see fit.

# Laptop Software

Now, we need an application that I can run on my MacBook Pro to control the sign. Let's keep it simple: this should be a Python app that can pair with the sign over Bluetooth and then has a straightforward way to send the message to the sign. 

When the app starts up, it should start the web server that I'll use to communicate with the app.

At startup it should also fetch current weather data from the web (using any open API for weather) for the location that the laptop will be in, which is Burlington, MA. Once it has paired with the sign, it should send the message to set the time on the sign and then send the message to set the weather data.

After that's done, it should send the clock mode message to the sign.

It should then present a web page (accessible on localhost)  where any one of the messages can be selected. There should be a field where I can specifiy the variables for the messages (e.g., how many minutes a meeting will be and whether it will be a "fun" meeting or not.) If we can manage it, I would like the app to communicate with my calendar and populate the fields for the meeting message if there is a meeting coming up.

