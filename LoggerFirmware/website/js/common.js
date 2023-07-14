function sendCommand(command, timeout) {
    console.log(`Sending command ${command} to logger with timeout ${timeout} s`);
}

function rebootLogger() {
    let command = 'restart';
    sendCommand(command, 3);
}