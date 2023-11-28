async function sendCommand(cmdstr, timeout = 10) {
    const url = 'http://192.168.4.1/command';
    let data = { command: cmdstr };
    let response = await fetch(url, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json;charset=utf-8'
            },
            body: JSON.stringify(data)
        });
    return response.json();
}

function rebootLogger() {
    let command = 'restart';
    sendCommand(command, 3);
}
