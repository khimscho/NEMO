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
        document.getElementById("metadata-output").innerHTML = contents.result;
        /* TODO: Send the contents of the read to the logger with "metadata ${contents.result}" */
    }
}