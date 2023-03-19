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
`clear()` - Clears the strip  
`show()` - Updates the strip (same as if draw=true)  
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
`err`: Any errors the ESP encounters  

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
    
    esp.clear(); // Clear the strip
    process.exit(0); // Exit the smal example
})();
```
