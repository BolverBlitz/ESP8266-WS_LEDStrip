const WebSocket = require('ws');
const client = require('dgram').createSocket('udp4');
const EventEmitter = require('events');

class ESP8266_RGB extends EventEmitter {
    /**
     * @param {String} ip 
     * @param {Number} [port] 
     */
    constructor(ip, port = 80) {
        super();
        this.ws = new WebSocket(`ws://${ip}:${port}`);

        this.remote_ip = ip;
        this.remote_port = port;

        this.isConnected = false;

        this.state_raw = false;
        this.state_udp = false;
        this.state_debug = false;

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
            state: "STATE",
            raw: "RAW",
            debug: "DEBUG",
            udp: "UDP"
        };

        this.pinMap = "WeMOSD1_R1";

        this.ws.on('message', (message) => {
            // Handle PIN Events
            if (message.toString().startsWith("7:")) {
                const revercedPIN = this.#reverceObject(this.PINS[this.pinMap]);
                this.emit('pin', { pin: revercedPIN[message.toString().split(":")[1].split(',')[0]], state: message.toString().split(":")[1].split(',')[1] });
                return;
            }
            // Handle State Events (Update the in class state)
            if (message.toString().startsWith("INFO:")) {
                message.toString().split(":")[1].split(",").forEach(state => {
                    let [key, value] = state.split('.');
                    value = value === 'true';

                    switch (key) {
                        case 'UDP':
                            this.state_udp = value;
                            break;
                        case 'RAW':
                            this.state_raw = value;
                            break;
                        case 'DEBUG':
                            this.state_debug = value;
                            break;
                    }
                });
                return;
            }
            // Handle Error Events
            if (message.toString().startsWith("ERROR:")) {
                const revercedPIN = this.#reverceObject(this.PINS[this.pinMap]);
                this.emit('err', { pin: revercedPIN[message.toString().split(":")[1].split(',')[0]], state: message.toString().split(":")[1].split(',')[1] });
                return;
            }
            // Handle RAW Events
            if (message.toString().startsWith("RAW:")) {
                this.emit('raw', { state: message.toString().split(":")[1] });
                return;
            }
            // Handle the rest
            this.emit('msg', message.toString());
        });

        this.ws.on('close', () => {
            this.isConnected = false;
            this.emit('ws_close');
        });

        this.ws.on('error', (error) => {
            this.isConnected = false;
            this.emit('ws_close');
            console.error(error);
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
        if (instruction in this.plainInstructions) return this.plainInstructions[instruction];
        return `${this.instructions[instruction]}:${start},${end},${draw},${r},${g},${b},${w}`;
    };

    /**
     * Sends a instruction
     * @param {String} instruction 
     * @returns 
     */
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

    /**
     * Sends a instruction and waits for the callback
     * @param {String} instruction instruction to send
     * @param {String} callback Expected callback String
     * @returns 
     */
    #sendInstructionCallback(instruction, callback) {
        return new Promise((resolve, reject) => {
            // Create Event Listener function
            const messageListener = (message) => {
                if (message.toString() === callback) {
                    this.ws.off('message', messageListener);
                    resolve();
                }
            };

            this.ws.send(instruction, (err) => {
                if (err) {
                    this.ws.off('message', messageListener);
                    reject(err);
                }
            });

            const timeoutTimeout = setTimeout(() => {
                this.ws.off('message', messageListener)
                clearTimeout(timeoutTimeout);
                reject(new Error('Timeout'));
            }, 5000);

            // Add Event Listener
            this.ws.on('message', messageListener);
        });
    }

    /**
     * Send a RGBW UDP packet
     * @param {Array} rgbw_values 
     */
    sendUDPPacket = (rgbw_values) => {
        if(!this.state_udp) throw new Error('UDP is not enabled');
        // Convert the RGBW array to a byte array
        const rgbw_bytes = rgbw_values.flatMap(value => {
            const [r, g, b, w] = value.split(',').map(Number);
            return [r, g, b, w];
        });
    
        let buffer = Buffer.alloc(1 + rgbw_bytes.length + 2);
        buffer[0] = 0xAA;
        rgbw_bytes.forEach((value, i) => {
            buffer[i + 1] = value;
        });
    
        let sum = 0; // Calculate the checksum
        for (let i = 0; i < buffer.length - 2; i++) {
            sum += buffer[i];
        }
        buffer[buffer.length - 2] = sum >> 8; // high byte
        buffer[buffer.length - 1] = sum & 0xFF; // low byte
    
        client.send(buffer, this.remote_port + 1, this.remote_ip, (err) => {
            if (err) console.error('Error sending UDP packet:', err);
        });
    }
    
    /**
     * Send a UDP instruction packet, you should always send a getState packet after a instruction packet to keel the state in sync
     * @param {String} instruction 
     */
    sendUDPInstructionPacket = (instruction) => {
        if(!this.state_udp) throw new Error('UDP is not enabled');
        const instruction_bytes = {
            'udp': 0xAB,
            'getState': 0xAC,
        }
    
        if(!instruction_bytes.hasOwnProperty(instruction)) throw new Error('Instruction not found');
    
        let buffer = Buffer.alloc(2);
        buffer[0] = instruction_bytes[instruction]; // Set the instruction byte
        buffer[1] = 0x00;
    
        // Senden Sie das Paket an das Zielgerät
        client.send(buffer, this.remote_port + 1, this.remote_ip, (err) => {
            if (err) console.error('Error sending UDP packet:', err);
        });
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.ws.on('open', () => {
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
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
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
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
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
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
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
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
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
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
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
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        const command = `${this.instructions["GPIO_PIN_STATE"]}:${pin},${state === true ? 1 : 0},0,0,0,0,0,0`;
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Clears the strip
     */
    clear = async () => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        const command = this.#generateInstruction('clear', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Shows the strip
    */
    show = async () => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        const command = this.#generateInstruction('show', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Get the current state of the ESP Settings
     */
    getState = async () => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible. Use sendUDPInstructionPacket("getState") instead');
        const command = this.#generateInstruction('state', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Switch DEBUG Mode
     */
    debug = async () => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        const command = this.#generateInstruction('debug', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    /**
     * Switch RAW Mode
     */
    raw = async () => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        const command = this.#generateInstruction('raw', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstruction(command);
        return command;
    }

    sendRaw = async (raw) => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        if(!this.state_raw) throw new Error('RAW is not enabled');
        await this.#sendInstruction(raw);
        return;
    }

    /**
     * Switch to UDP Mode
     */
    udpSafe = async () => {
        if(this.state_udp) throw new Error('UDP is enabled, sending WS commands is not possible');
        const command = this.#generateInstruction('udp', 0, 0, 0, 0, 0, 0, 0);
        await this.#sendInstructionCallback(command, "UDP:ON");
        this.state_udp = true;
        return command;
    }
}

module.exports = ESP8266_RGB;