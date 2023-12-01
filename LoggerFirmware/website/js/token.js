function populateToken(data) {
    if (data.messages[0] === '') {
        document.getElementById('token-display').textContent = 'Not set';
    } else {
        document.getElementById('token-display').textContent = data.messages[0];
    }
}

function resetToken() {
    const token = document.getElementById('add-item').value;
    document.getElementById('add-item').value = '';
    sendCommand(`token ${token}`).then((data) => {
        populateToken(data);
    });
}

function bootstrapToken() {
    const boot = () => {
        sendCommand('token').then((data) => {
            populateToken(data);
        });
    }
    after(500, boot);
}