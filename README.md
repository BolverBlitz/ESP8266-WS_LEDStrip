# ESP-Websocket-JS
Easy to use JS lib to controle your ESP8266 via WS.
It supports basic GPIO usage and a neopixel LED (RGBW) Strip.

## Example
```js
const ESP8266_RGB = require('./index.js');
const esp = new ESP8266_RGB('ESP IP ADDRESS', ESP PORT);
(async function (){
 try {
   await esp.connect() // Connect to the ESP (This will emit a event when connected)
   esp.setStripWhite(100, true); // Set Strip white
 } catch (e) {
   console.log(e)
 }
})();
```
## Why is the draw option optional?
If more than 1 command needs to be executed, or multiple pixels are modifyed its good to do that without updating the strip for every command.
Updating the strip costs quite a lot of performance.

## Methods  
### Neopixel (LED Strip)  
`setPixel(pixel<Number>, rgb<RGBWObject>, draw<Boolean>)` - Set the color of a single pixel  
`setLine(x<Number>, y<Number>, rgb<RGBWObject>, draw<Boolean>)` - Sets the color of all pixels from x to y  
`setStrip(rgb<RGBWObject>, draw<Boolean>)` - Sets the color of the entire strip  
`setStripWhite(w<Number>,  draw<Boolean>)` - Sets the strip wo its w color (r:0,g:0,g:0,w:w)  
`sendRaw(<String>)`- Send a string of RGBW values to the ESP (r,g,b,w,r,b,g,w,...)
`clear()` - Clears the strip   
`show()` - Updates the strip (same as if draw=true)  
`raw()` - Switch RAW Mode  
`udpSafe()` - Switch to UDP Mode  
`sendUDPPacket(<String>)` - Send a UDP Packet to the ESP (r,g,b,w,r,b,g,w,...)  
`sendUDPInstructionPacket(<String>)` - Send a UDP Instruction Packet  
### GPIO  
`setGPIO_PIN_Mode(PIN<String>, MODE<Boolean>)` - Set a GPIO pin to input (false) or output (true)  
`setGPIO_PIN_State(PIN<String>, MODE<Boolean>)` - Set a GPIO pin state HIGH (true) or LOW (false)  
### Misc  
`connect()` - Connects to the ESP  
`color(Color<String>)` - Converts a color like "red" into its RGBW values  
`rgb(r<Number>,g<Number>,b<Number>,w<Number>)` - Converts to a RGBW object, used by the lib  
`setPinMap(pinMap<BoardName>)` - Ajusts the GPIO Pin names based on the board used  

## Events
3 Events are emited (msg, pin, err).  
`msg`: Any message (like connected notificatin)  
`pin`: Updates on a GPIO pin status  
`raw`: Provides information if the raw mode is on/off  
`err`: Any errors the ESP encounters  

## Modes explained
### Default Mode
You get all advantages of webcockets you would expect.  
In default you can only set a pixel, draw a line or set all LEDs with one command.
### RAW Mode
You get all advantages of webcockets you would expect.  
In RAW Mode you can send a string of RGBW values to the ESP (r,g,b,w,r,b,g,w,...).  
The ESP will still handle all normal commands send in default mode.
You can always switch back to default mode by calling `raw()`.  
### UDP Mode
WARNING: This mode can cause the ESP to crash if you flood it with data. Please only send up to 240 Packets per second (If 160MHZ is used). A sidenote, transmitting data will only be checked with a sum, therfor it can´t detect bitflips. UDP data is processed in the order of arrival, not in the order of sending.
In UDP Mode you can send a Array of RGBW values to the ESP (["r,g,b,w"],["r,g,b,w"],...) via UDP.  
The ESP will keel WS connection open but will not handle any commands send via WS. They will build up in a queue and and will be executed once UDP Mode is disabled, however this can fill up the ESPs memory. Therfor its not allowed to send data via WS and you get a error if you try.  
You can always switch back to default mode by sending a special UDP packet via `sendUDPInstructionPacket(<String>)`.

## Large Example
You can use a simple function to replicate delay() or wait() from other languages.
```js
const delay = (time) => new Promise(resolve => setTimeout(resolve, time));
```
Examples:
```js
const ESP8266_RGB = require('./index.js');
const esp = new ESP8266_RGB('192.168.0.10', 81);
const delay = (time) => new Promise(resolve => setTimeout(resolve, time));
let response;

// Add our event handlers
esp.on('msg', (message) => {
    console.log(`Got from ESP: ${message}`);
});

esp.on('pin', (pin) => {
    console.log(`Got from ESP: ${pin.pin} is ${pin.state}`);
});

esp.on('err', (pin) => {
    console.log(`Got from ESP: ${pin.pin} Test: ${pin.state}`);
});

(async () => {
    await esp.connect(); // Connect to the ESP

    response = await esp.setPixel(0, esp.color("magenta"), true); // Set a pixel to magenta. The function returns the command executed
    console.log(`Sent to ESP: ${response}`);  // Print the command

    await delay(1000); // Wait 1 second (1000ms)

    response = await esp.setPixel(0, esp.rgb(255, 0, 0, 0), true); // Set a pixel to red (255,0,0,0. The function returns the command executed
    console.log(`Sent to ESP: ${response}`);  // Print the command

    await delay(1000); // Wait 1 second (1000ms)

    response = await esp.setLine(0, 10, esp.rgb(0, 0, 255, 0), true); // Set pixel 0-10 to blue.The function returns the command executed
    console.log(`Sent to ESP: ${response}`);  // Print the command

    await delay(1000); // Wait 1 second (1000ms)

    response = await esp.setStrip(esp.rgb(0, 50, 50, 0), true); // Set the entire strip to cyan.The function returns the command executed
    console.log(`Sent to ESP: ${response}`);  // Print the command

    await delay(1000); // Wait 1 second (1000ms)

    response = await esp.raw();

    console.log(`Sent to ESP: ${response}`);  // Print the command

    await delay(1000); // Wait 1 second (1000ms)

    const rawRot = "50,0,0,0"
    const rawGün = "0,50,0,0"
    const rawBlau = "0,0,50,0"
    const colorObject = {
        0: [rawRot, rawGün, rawBlau],
        1: [rawGün, rawBlau, rawRot],
        2: [rawBlau, rawRot, rawGün,]
    }

    let LED = Array(100);

    for (let rounds = 0; rounds < 1000; rounds++) {
        for (let y = 0; y < 100; y++) {
            LED[y] = colorObject[rounds % 3][y % 3];
        }
        esp.sendRaw(LED.join(","));
        await delay(1);
    }

    await delay(1000); // Wait 1 second (1000ms)

    response = await esp.raw();
    console.log(`Sent to ESP: ${response}`);  // Print the command

    await delay(2000); // Wait 1 second (1000ms)
    
    esp.clear(); // Clear the strip
    process.exit(0); // Exit the smal example
})();
```
