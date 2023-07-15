async function transferData() {
    const fileID = document.getElementById("file-id").value;
    let data = sendCommand(`transfer ${fileID}`);
    const options = {
        suggestedName: `wibl-raw.${fileID}.wibl`
    }
    const fileHandle = await window.showSaveFilePicker(options);
    const fileStream = await fileHandle.createWritable();
    await fileStream.write(new Blob([data], {type: 'application/octet-stream'}));
    await fileStream.close();
}
