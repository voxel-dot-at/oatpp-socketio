import { Server } from "socket.io";

const io = new Server({ /* options */ });

io.on("connection", (socket) => {
    console.log("CONNECT ", socket.id)
            socket.emit('cam', {"connection":true, d: Date.now()})
            setInterval(()=>{
                socket.emit('cam', {d: Date.now()})
                //socket.send( JSON.stringify({d: Date.now()}))
            }, 5*1000);

});

io.listen(8000);