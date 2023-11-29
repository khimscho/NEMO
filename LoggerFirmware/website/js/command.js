let commandText = document.getElementById("command");
commandText.addEventListener("keypress", function(event) {
    if (event.key === "Enter") {
        event.preventDefault();
        document.getElementById("command-run").click();
    }
});

function runCommand() {
    const command = document.getElementById("command").value;
    sendCommand(command).then((data) => {
        document.getElementById("command-output").value = data;
        document.getElementById("command").value = '';
    });
}
