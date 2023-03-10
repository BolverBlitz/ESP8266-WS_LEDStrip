const ESP8266_RGB = require('./index.js');

const esp = new ESP8266_RGB('192.168.88.171', 81);

esp.on('msg', (message) => {
    console.log(`Got from ESP: ${message}`);
});

esp.on('pin', (pin) => {
    console.log(`Got from ESP: ${pin.pin} is ${pin.state}`);
});

esp.on('err', (pin) => {
    console.log(`Got from ESP: ${pin.pin} Test: ${pin.state}`);
});

let response

function delay(time) {
    return new Promise(resolve => setTimeout(resolve, time));
}

(async () => {
    await esp.connect();

    //response = await esp.setGPIO_PIN_Mode(esp.PINS.WeMOSD1_R1.D5, true);
    // response = await esp.setGPIO_PIN_Mode(0, true);
    //console.log(`Sent to ESP: ${response}`);

    response = await esp.setGPIO_PIN_State(esp.PINS.WeMOSD1_R1.D3, true);

    console.log(`Sent to ESP: ${response}`);

    /*
    esp.setStripWhite(0, true);


    response = await esp.setPixel(0, esp.color("magenta"), true);
    console.log(`Sent to ESP: ${response}`)

    await delay(1000);

    response = await esp.setPixel(0, esp.rgb(255, 0, 0, 0), true);
    console.log(`Sent to ESP: ${response}`)

    await delay(1000);

    response = await esp.setLine(0, 10, esp.rgb(0, 0, 255, 0), true);
    console.log(`Sent to ESP: ${response}`)

    await delay(1000);

    response = await esp.setStrip(esp.rgb(0, 50, 50, 0), true);
    console.log(`Sent to ESP: ${response}`)


    await delay(1000);
    */
    //esp.clear();

    //process.exit(0);
})();