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

function populateAlgTable() {
    //const rawData = sendCommand('algorithm');
    rawData = `{
        "count": 2,
        "algorithm": [
            {
                "name": "deduplicate",
                "parameters": ""
            },
            {
                "name": "uncertainty",
                "parameters": "0.25,0.0135,0.95"
            }
        ]
    }`;
    const data = JSON.parse(rawData);
    const headerRow = createAlgTableHeader();
    document.getElementById("alg-table").replaceChildren(headerRow);
    for (let n = 0; n < data.count; ++n) {
        const row = createAlgTableRow(data.algorithm[n].name, data.algorithm[n].parameters);
        document.getElementById("alg-table").appendChild(row);
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
    sendCommand(`algorithm ${algName} ${algParams}`);
    populateAlgTable();
}

function clearAlgList() {
    sendCommand('algorithm none'); // Remove all algorithms from the logger
    populateAlgTable(); // This regenerates the list, clearing the local table
}
