function sendCommand(command, timeout = 10) {
    console.log(`Sending command |${command}| to logger with timeout ${timeout} s`);
    return 'no-data-returned';
}

function rebootLogger() {
    let command = 'restart';
    sendCommand(command, 3);
}

function roundSize(rawSize) {
    const rounded = Math.round(rawSize*100)/100;
    return rounded;
}

function translateSize(byteCount) {
    let dataSize;

    if (byteCount > 1024*1024*1024) {
        dataSize = roundSize(byteCount/(1024*1024*1024)) + ' GB';
    } else if (byteCount > 1024*1024) {
        dataSize = roundSize(byteCount/(1024*1024)) + ' MB';
    } else if (byteCount > 1024) {
        dataSize = roundSize(byteCount/1024) + ' kB';
    } else {
        dataSize = byteCount + ' B';
    }
    return dataSize;
}

function assembleSummaryHeader(name) {
    const headerRow = document.createElement("tr");
    const header = document.createElement("th");
    header.setAttribute("colspan", "2");
    header.textContent = name;
    headerRow.appendChild(header);
    return headerRow;
}

function assembleDataHeader(name) {
    const headerRow = document.createElement("tr");
    const header = document.createElement("th");
    header.setAttribute("colspan", "4");
    header.textContent = name;
    headerRow.appendChild(header);
    return headerRow;
}

function assembleDetailHeader() {
    const headerRow = document.createElement('tr');
    const idCol = document.createElement('th');
    idCol.textContent = 'ID';
    const sizeCol = document.createElement('th');
    sizeCol.textContent = 'Size';
    const md5Col = document.createElement('th');
    md5Col.textContent = 'MD5 Checksum';
    headerRow.appendChild(idCol);
    headerRow.appendChild(sizeCol);
    headerRow.appendChild(md5Col);
    return headerRow;
}

function assembleSummaryRow(name, version) {
    let row = document.createElement("tr");
    let nameEl = document.createElement("td");
    nameEl.textContent = name;
    let versionEl = document.createElement("td");
    versionEl.textContent = version;
    row.appendChild(nameEl);
    row.appendChild(versionEl);
    return row;
}

function assembleDataRow(id, source, time, data) {
    let row = document.createElement('tr');
    let idEl = document.createElement('td');
    idEl.textContent = id;
    let sourceEl = document.createElement('td');
    sourceEl.textContent = source;
    let timeEl = document.createElement('td');
    timeEl.textContent = time;
    let dataEl = document.createElement('td');
    dataEl.textContent = data;
    row.appendChild(idEl);
    row.appendChild(sourceEl);
    row.appendChild(timeEl);
    row.appendChild(dataEl);
    return row;
}

function assembleDetailRow(id, len, md5) {
    let row = document.createElement('tr');
    let idEl = document.createElement('td');
    idEl.textContent = id;
    let sizeEl = document.createElement('td');
    sizeEl.textContent = translateSize(len);
    let md5El = document.createElement('td');
    md5El.textContent = md5;
    row.appendChild(idEl);
    row.appendChild(sizeEl);
    row.appendChild(md5El);
    return row; 
}

function updateStatus(tablePrefix) {
    //const rawData = sendCommand('status');
    const rawData = `{
        "version": {
            "commandproc": "1.2.0",
            "nmea0183": "0.0.1",
            "nmea2000": "0.0.1",
            "imu": "0.0.1",
            "serialiser": "0.1"
        },
        "elapsed": 12.345,
        "webserver": {
            "current": "AP-Enabled",
            "boot": "Station-Active"
        },
        "data": {
            "nmea0183": {
                "count": 3,
                "detail": [
                    {
                        "name": "Depth",
                        "tag": "DBT",
                        "time": 24,
                        "time_units": "s",
                        "display": "$SDDBT,32.9,f,10.0,M,5.5,F*0F"
                    },
                    {
                        "name": "Position",
                        "tag": "GGA",
                        "time": 15,
                        "time_units": "s",
                        "display": "$GPGGA,000058.000,4300.000193,N,07400.999807,W,3,12,1.0,-19.5,M,22.5,M,0.0,0000*71"
                    },
                    {
                        "name": "Time",
                        "tag": "ZDA",
                        "time": 1.04,
                        "time_units": "s",
                        "display": "$GPZDA,000058.000,01,01,2020,00,00*5B"
                    }
                ]
            },
            "nmea2000":  {
                "count": 3,
                "detail": [
                    {
                        "name": "Depth",
                        "tag": "Depth",
                        "time": 24,
                        "time_units": "s",
                        "display": "10.0m"
                    },
                    {
                        "name": "Position",
                        "tag": "GNSS",
                        "time": 12,
                        "time_units": "s",
                        "display": "43 00.000193 N, 074 00.999807 W"
                    },
                    {
                        "name": "Time",
                        "tag": "SystemTime",
                        "time": 1.001,
                        "time_units": "s",
                        "display": "2020-01-01/00:00:58.000 Z"
                    }
                ]
            }
        },
        "files": {
            "count": 3,
            "detail": [
                {
                    "id": 0,
                    "len": 221023,
                    "md5": "1a05a697d5b021a43896ae103e391be4"
                },
                {
                    "id": 1,
                    "len": 10263234,
                    "md5": "eb9d54fb412e8f328aedaab7794275a2"
                },
                {
                    "id": 2,
                    "len": 8956065,
                    "md5": "0b29903bc6fab962162132f7bc694192"
                }
            ]
        }
    }`;
    const data = JSON.parse(rawData);
    const versionsTable = tablePrefix + '-versions';
    const statsTable = tablePrefix + '-stats';
    const n0183Table = tablePrefix + '-nmea0183';
    const n2000Table = tablePrefix + '-nmea2000';
    const detailTable = tablePrefix + '-detail';

    let versions = document.getElementById(versionsTable);
    versions.replaceChildren(assembleSummaryHeader("Versions"));
    versions.appendChild(assembleSummaryRow("Command Processor", data.version.commandproc));
    versions.appendChild(assembleSummaryRow("NMEA0183", data.version.nmea0183));
    versions.appendChild(assembleSummaryRow("NMEA2000", data.version.nmea2000));
    versions.appendChild(assembleSummaryRow("IMU", data.version.imu));
    versions.appendChild(assembleSummaryRow("Serialiser", data.version.serialiser));

    let totalFileSize = 0;
    for (let n = 0; n < data.files.count; ++n) {
        totalFileSize += data.files.detail[n].len;
    }

    let stats = document.getElementById(statsTable);
    stats.replaceChildren(assembleSummaryHeader("Status"));
    stats.appendChild(assembleSummaryRow("Elapsed Time", data.elapsed + ' s'));
    stats.appendChild(assembleSummaryRow("Webserver Status Current", data.webserver.current));
    stats.appendChild(assembleSummaryRow("Webserver Status Boot", data.webserver.boot));
    stats.appendChild(assembleSummaryRow("Files on Logger", data.files.count));
    stats.appendChild(assembleSummaryRow("Total Size", translateSize(totalFileSize)));

    let nmea0183 = document.getElementById(n0183Table);
    if (nmea0183 !== null) {
        nmea0183.replaceChildren(assembleDataHeader("NMEA0183 Data"));
        console.log('found ' + data.data.nmea0183.count + ' NMEA0183 status elements');
        for (let n = 0; n < data.data.nmea0183.count; ++n) {
            nmea0183.appendChild(assembleDataRow(
                data.data.nmea0183.detail[n].name,
                data.data.nmea0183.detail[n].tag,
                data.data.nmea0183.detail[n].time + ' ' + data.data.nmea0183.detail[n].time_units,
                data.data.nmea0183.detail[n].display));
        }
    }
    
    let nmea2000 = document.getElementById(n2000Table);
    if (nmea2000 !== null) {
        nmea2000.replaceChildren(assembleDataHeader("NMEA2000 Data"));
        for (let n = 0; n < data.data.nmea2000.count; ++n) {
            nmea2000.appendChild(assembleDataRow(
                data.data.nmea2000.detail[n].name,
                data.data.nmea2000.detail[n].tag,
                data.data.nmea2000.detail[n].time + ' ' + data.data.nmea2000.detail[n].time_units,
                data.data.nmea2000.detail[n].display));
        }
    }

    let detail = document.getElementById(detailTable);
    if (detail !== null) {
        detail.replaceChildren(assembleDetailHeader());
        for (let n = 0; n < data.files.count; ++n) {
            detail.appendChild(assembleDetailRow(data.files.detail[n].id, data.files.detail[n].len, data.files.detail[n].md5));
        }
    }
}

function updateIndexStatus() {
    updateStatus('index');
}

function updateStatusData() {
    updateStatus('status');
}