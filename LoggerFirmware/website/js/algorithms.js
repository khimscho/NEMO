function createAlgTableHeader() {
    let row = document.createElement('tr');
    let nameHdr = document.createElement('th');
    nameHdr.textContent = 'Name';
    let paramHdr = document.createElement('th');
    paramHdr.textContent = 'Parameters';
    row.appendChild(nameHdr);
    row.appendChild(paramHdr);
    return row;
}

function createAlgTableRow(algName, algParams) {
    let row = document.createElement('tr');
    let nameEl = document.createElement('td');
    nameEl.textContent = algName;
    let paramEl = document.createElement('td');
    if (algParams.length === 0) {
        paramEl.textContent = 'N/A';
    } else {
        paramEl.textContent = algParams;
    }
    row.appendChild(nameEl);
    row.appendChild(paramEl);
    return row;
}

function populateAlgTable(data) {
    const headerRow = createAlgTableHeader();
    document.getElementById("alg-table").replaceChildren(headerRow);
    if (data.count === 0) {
        let row = document.createElement('tr');
        let elem = document.createElement('td');
        elem.setAttribute('colspan', '2');
        elem.textContent = 'No algorithms stored in logger';
        row.appendChild(elem);
        document.getElementById('alg-table').appendChild(row);
    } else {
        for (let n = 0; n < data.count; ++n) {
            const row = createAlgTableRow(data.algorithm[n].name, data.algorithm[n].parameters);
            document.getElementById("alg-table").appendChild(row);
        }
    }
}

function addAlgorithm() {
    let algName = document.getElementById('alg-name').value;
    let algParams = document.getElementById('alg-params').value;
    document.getElementById('alg-name').value = '';
    document.getElementById('alg-params').value = '';
    if (algParams === 'N/A') {
        algParams = '';
    }
    sendCommand(`algorithm ${algName} ${algParams}`).then((data) => {
        populateAlgTable(data);
    });
}

function clearAlgList() {
    sendCommand('algorithm none').then((data) => {
        populateAlgTable(data);
    });
}

function bootstrapAlgTable() {
    const boot = () => {
        sendCommand('algorithm').then((data) => {
            populateAlgTable(data);
        });
    }
    after(500, boot);
}