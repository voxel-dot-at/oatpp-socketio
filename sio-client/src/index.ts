import { io, Socket } from "socket.io-client";


class Runner {
    async run() {
        let sock: Socket = io('ws://192.168.2.128:8000/', {
            // upgrade: false,
            retries: 1,
            timeout: 60 * 1000,
            requestTimeout: 60 * 1000
        })

        sock.on("connect", () => {
            console.log("SOCK CONNECTED", sock.id); // "G5p5..."

            setInterval(()=>{
                // sock.emit('foo', {d: Date.now()})
                sock.send( JSON.stringify({d: Date.now()}))
            }, 5*1000);
        });
    }
}


let r = new Runner;

r.run();
