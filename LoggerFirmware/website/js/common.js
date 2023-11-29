async function sendCommand(cmdstr, timeout = 10) {
    const url = 'http://192.168.4.1/command';
    let data = new FormData();
    data.append("command", cmdstr);
    let response = await fetch(url, {
            method: 'POST',
            body: data
        });
    return response.json();
}

function rebootLogger() {
    let command = 'restart';
    sendCommand(command, 3);
}
