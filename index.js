const WebSocket = require('ws');
const EventEmitter = require('events');

class ESP8266_RGB extends EventEmitter {
    /**
     * @param {String} ip 
     * @param {Number} [port] 
     */
    constructor(ip, port = 80) {
        super();
        this.ws = new WebSocket(`ws://${ip}:${port}`);

        this.isConnected = false;

        this.instructions = {
            setPixel: 0,
            setLine: 1,
            setStrip: 2,
            setStripWhite: 3,
            GPIO_PIN_MODE: 8,
            GPIO_PIN_STATE: 9,
        };

        this.plainInstructions = {
            clear: "CLEAR",
            show: "SHOW",
            raw: "RAW"
        };

        this.pinMap = "WeMOSD1_R1";

        this.ws.on('message', (message) => {
            if(message.toString().startsWith("7:")) {
                const revercedPIN = this.#reverceObject(this.PINS[this.pinMap]);
                this.emit('pin', {pin: revercedPIN[message.toString().split(":")[1].split(',')[0]], state: message.toString().split(":")[1].split(',')[1]});
                return;
            }

            if(message.toString().startsWith("ERROR:")) {
                const revercedPIN = this.#reverceObject(this.PINS[this.pinMap]);
                this.emit('err', {pin: revercedPIN[message.toString().split(":")[1].split(',')[0]], state: message.toString().split(":")[1].split(',')[1]});
                return;
            }

            if(message.toString().startsWith("RAW:")) {
                this.emit('raw', {state: message.toString().split(":")[1]});
                return;
            }
            
            this.emit('msg', message.toString());
        });
    }

    PINS = {
        "WeMOSD1_R1": {
            "D0": 2,
            "D1": 3,
            "D2": 4,
            "RX": 0,
            "TX": 1,
            "D3": 8,
            "D5": 5,
            "D6": 6,
            "D7": 7,
        }
    }

    #reverceObject = obj => Object.fromEntries(Object.entries(obj).map(([k, v]) => [v, k]))

    #generateInstruction(instruction, start, end, draw, r, g, b, w) {
        if(instruction in this.plainInstructions) return this.plainInstructions[instruction];
        return `${this.instructions[instruction]}:${start},${end},${draw},${r},${g},${b},${w}`;
    };

    #sendInstruction(instruction) {
        return new Promise((resolve, reject) => {
            this.ws.send(instruction, (error) => {
                if (error) {
                    reject(error);
                } else {
                    resolve();
                }
            });
        });
    };

    connect() {
        return new Promise((resolve, reject) => {
            this.ws.on('open', (message) => {
                this.isConnected = true;
                resolve(true);
            });
        });
    };

    /**
     * Tells the class to translate PINs to the given board
     * @param {BoardName} pinMap 
     */
    setPinMap = (pinMap) => {
        this.pinMap = pinMap;
    }

    /**
     * Converts a commen color (red, green, blue, white, yellow, cyan, magenta, orange, purple, pink, brown, black) to a rgb object
     * @param {String} color 
     * @returns 
     */
    color = (color) => {
        switch (color) {
            case 'red':
                return this.rgb(255, 0, 0, 0);
            case 'green':
                return this.rgb(0, 255, 0, 0);
            case 'blue':
                return this.rgb(0, 0, 255, 0);
            case 'white':
                return this.rgb(255, 255, 255, 0);
            case 'yellow':
                return this.rgb(255, 255, 0, 0);
            case 'cyan':
                return this.rgb(0, 255, 255, 0);
            case 'magenta':
                return this.rgb(255, 0, 255, 0);
            case 'orange':
                return this.rgb(255, 132, 0, 0);
            case 'purple':
                return this.rgb(128, 0, 128, 0);
            case 'black':
                return this.rgb(0, 0, 0, 0);
            default:
                return this.rgb(0, 0, 0, 0);
        }
    }

    /**
     * Generate a rgb object
     * @param {Number} r 
     * @param {Number} g 
     * @param {Number} b 
     * @param {Number} w 
     * @returns 
     */
    rgb(r, g, b, w) {
        return { r, g, b, w };
    }

    /**
     * Set the color of one pixel, to se multiple pixels disable draw and send draw command after all pixels are set
     * @param {Number} x Start
     * @param {Object} rgb {r,g,b,w}
     * @param {Boolean} draw 
     * @returns {String} command
     */
    setPixel = async (x, rgb, draw = true) => {
        const { r, g, b, w } = rgb;
        const command = this.#generateInstruction('setPixel', x, 0, draw === true ? 1 : 0, r, g, b, w)
        await this.#sendInstruction(command);
        return command;
    };

    /**
     * Set the color of a line of pixels, to se multiple lines disable draw and send draw command after all lines are set
     * @param {Number} x Start
     * @param {Number} y End
     * @param {Object} rgb {r,g,b,w}
     * @param {Boolean} draw
     * @returns {String} command
     */
    setLine = async (x, y, rgb, draw = true) => {
        const { r, g, b, w } = rgb;
        const command = this.#generateInstruction('setLine', x, y, draw === true ? 1 : 0, r, g, b, w)
        await this.#sendInstruction(command);
        return command;
    };

    /**
     * Set the color of the whole strip
     * @param {Object} rgb {r,g,b,w}
     * @param {*} draw 
     * @returns {String} command
     */
    setStrip = async (rgb, draw = true) => {
        const { r, g, b, w } = rgb;
        const command = this.#generateInstruction('setStrip', 0, 0, draw === true ? 1 : 0, r, g, b, w)
        await this.#sendInstruction(command);
        return command;
    };

    /**
     * Set the white value of the whole strip
     * @param {Number} w White value
     * @param {Boolean} draw 
     * @returns 
     */
    setStripWhite = async (w, draw = true) => {
        const command = this.#generateInstruction('setStripWhite', 0, 0, draw === true ? 1 : 0, 0, 0, 0, w)
        await this.#sendInstruction(command);
        return command;
    };

    /**
     * Set the pin mode of a GPIO pin (Output or Input)
     * @param {PIN} pin 
     * @param {Boolean} mode 
     * @returns 
     */
    setGPIO_PIN_Mode = async (pin, mode) => {
        const command = `${this.instructions["GPIO_PIN_MODE"]}:${pin},${mode === true ? 1 : 0},0,0,0,0,0,0`;
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Set the state of a GPIO pin (High or Low)
     * @param {PIN} pin 
     * @param {Boolean} state 
     * @returns 
     */
    setGPIO_PIN_State = async (pin, state) => {
        const command = `${this.instructions["GPIO_PIN_STATE"]}:${pin},${state === true ? 1 : 0},0,0,0,0,0,0`;
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Clears the strip
     */
    clear = async () => {
        const command = this.#generateInstruction('clear', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Shows the strip
    */
    show = async () => {
        const command = this.#generateInstruction('show', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Switch RAW Mode
     */
    raw = async () => {
        const command = this.#generateInstruction('raw', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    sendRaw = async (raw) => {
        await this.#sendInstruction(raw);
        return;
    }
}

module.exports = ESP8266_RGB;