function populateFilterTable(data) {
    let display = '';
    if (data.count == 0) {
        display = 'All';
    } else {
        for (let n = 0; n < data.count; ++n) {
            display += data.ids[n] + ' ';
        }
    }
    document.getElementById('filter-list').textContent = display;
}

function addFilter() {
    const rawID = document.getElementById('add-item').value;
    const filterID = rawID.toUpperCase();
    document.getElementById('add-item').value = '';
    sendCommand(`accept ${filterID}`).then((data) => {
        populateFilterTable(data);
    });
}

function clearFilterList() {
    sendCommand('accept all').then((data) => {
        populateFilterTable(data);
    });
}

function bootstrapFilterTable() {
    const boot = () => {
        sendCommand('accept').then((data) => {
            populateFilterTable(data);
        });
    }
    after(500, boot);
}