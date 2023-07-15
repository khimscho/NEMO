/* onUpload() is used to extract the file for metadata that the user has specified in the
 * input element, and loads the contents for transmission to the logger.
*/
function onUpload() {
    const selectedFile = document.getElementById("metadata").files[0];
    let contents = new FileReader();
    contents.readAsText(selectedFile);
    contents.onerror = function() {
        console.log(contents.error);
    }
    contents.onload = function() {
        let dict = JSON.parse(contents.result);
        let mapDict = JSON.stringify(dict);
        sendCommand('metadata ' + mapDict);
        let data = sendCommand('metadata');
        // Pretty-print the output
        data = `{
            "platform": {
                "uniqueID": "UNHJHC-RVGS",
                "shipname": "R/V Gulf Surveyor"
            }
        }`;
        dict = JSON.parse(data);
        mapDict = JSON.stringify(dict, null, 2);
        document.getElementById("metadata-output").innerHTML = mapDict;
    }
}