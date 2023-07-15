function populateFilterTable() {
    //const rawData = sendCommand('accept');
    const rawData = `{
        "count": 5,
        "accepted": [
            "GGA",
            "ZDA",
            "GGL",
            "RMC",
            "VTG"
        ]
    }`;
    const data = JSON.parse(rawData);
    let display = '';
    if (data.count == 0) {
        display = 'All';
    } else {
        for (let n = 0; n < data.count; ++n) {
            display += data.accepted[n] + ' ';
        }
    }
    document.getElementById('filter-list').textContent = display;
}

function addFilter() {
    const rawID = document.getElementById('add-item').value;
    const filterID = rawID.toUpperCase();
    document.getElementById('add-item').value = '';
    sendCommand(`accept ${filterID}`);
    populateFilterTable();
}

function clearFilterList() {
    sendCommand('accept all');
    populateFilterTable();
}
