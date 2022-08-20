//server
const serverAddress = "localhost:9001/";
//chat client protocol constants
const ccpSeparator = "::";
const ccpPrivateMessageCmd = "PRIVATE_MESSAGE";
const ccpSetNameCmd = "SET_NAME";

//access to DOM textarea - user debug console
function writeToDebugConsole(text) {
    let debugConsole = document.getElementById("debugConsole");
    debugConsole.value += text + '\n';
    debugConsole.scrollTop = debugConsole.scrollHeight;
}

//send message handler
function sendMessage() {
    if (socket.readyState === WebSocket.OPEN) {
        //receiver ID
        let receiverID = document.getElementById("receiverIDInput").value;
        //set name
        let senderName = document.getElementById("userName").value;
        if (senderName.length) {
            let message = ccpSetNameCmd + ccpSeparator + receiverID + ccpSeparator + senderName;
            socket.send(message);
            writeToDebugConsole(message);
        } else {
            writeToDebugConsole("ERROR: input your name please.");
            return;
        }
        //send message				
        let messageToReceiver = document.getElementById("userMessageInput").value;
        let message = ccpPrivateMessageCmd + ccpSeparator + receiverID + ccpSeparator + messageToReceiver;
        socket.send(message);
        writeToDebugConsole(message);
    } else {
        writeToDebugConsole("ERROR: no connection to server.");
    }
}

//receive message handler
function receiveMessage(msg) {
    let chatMainWindow = document.getElementById("chatMainWindow");
    chatMainWindow.value += msg.data + '\n';
    writeToDebugConsole(msg.data);
}


//WebSocket connect
var socket = new WebSocket("ws://" + serverAddress);
setTimeout(function () {
    if (socket.readyState === WebSocket.OPEN) {
        //OK message
        writeToDebugConsole("CONNECTED TO SERVER: " + serverAddress);
        //set receive message handler
        socket.onmessage = receiveMessage;
    } else {
        writeToDebugConsole("ERROR: can't connect to server " + serverAddress);
    }
},
    2000);