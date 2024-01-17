/*! @file wibl-monitor.go
 * @brief Demonstration upload server for WIBL loggers with auto-upload enabled.
 *
 * Although it is generally the case that WIBL loggers run without internet connection (most
 * small boats of opportunity don't have always-on connectivity), there are some use cases where
 * connectivity may be available, and the logger can upload files automatically to a server
 * (at which point it can be proxied into the S3 bucket for processing).  This code provides
 * the minimal core functionality for the server side of the protocol (the client side is in the
 * logger firmware) as a demonstration of what's required for an industrial-grade server
 * implementation.
 *
 * Copyright (c) 2024, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
Wibl-monitor demonstrates the server end of the WIBL logger upload protocol.
The code generates an HTTP server with two end-points:
  - checkin, which is used by loggers to report status information (and check the server is accessible)
  - update, which is used by loggers to transfer files for processing

Usage:

	wibl-monitor [flags]

The flags are:

	-config
		Specify a JSON format file to configure the server

Without flags, the code generates a default configuration for the server, typically
bringing it up on a non-constrained port (see support/config.go for details).
*/
package main

import (
	"crypto/md5"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"ccom.unh.edu/wibl-monitor/src/api"
	"ccom.unh.edu/wibl-monitor/src/support"
)

func main() {
	log.SetFlags(log.Lmicroseconds | log.Ldate)
	fs := flag.NewFlagSet("monitor", flag.ExitOnError)
	configFile := fs.String("config", "", "Filename to load JSON configuration")

	if err := fs.Parse(os.Args[1:]); err != nil {
		support.Errorf("failed to parse command line parameters (%v)\n", err)
		os.Exit(1)
	}

	var config *support.Config
	if len(*configFile) > 0 {
		var err error
		config, err = support.NewConfig(*configFile)
		if err != nil {
			support.Errorf("failed to generate configuration from %q (%v)\n", *configFile, err)
			os.Exit(1)
		}
	} else {
		config = support.NewDefaultConfig()
	}

	address := fmt.Sprintf(":%d", config.API.Port)

	mux := http.NewServeMux()
	mux.HandleFunc("/", syntax)
	mux.HandleFunc("/checkin", support.BasicAuth(status_updates))
	mux.HandleFunc("/update", support.BasicAuth(file_transfer))

	srv := &http.Server{
		Addr:         address,
		Handler:      mux,
		IdleTimeout:  time.Minute,
		ReadTimeout:  10 * time.Second,
		WriteTimeout: 30 * time.Second,
	}

	log.Printf("starting server on %s", srv.Addr)
	err := srv.ListenAndServeTLS("./localhost.pem", "./localhost-key.pem")
	log.Fatal(err)
}

// Generate a list of the end-points that the server provides.
func syntax(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "checkin\n")
	fmt.Fprintf(w, "update\n")
}

// Accept a status message from the logger client (which should list all of the files on the logger,
// along with other status information like the uptime, firmware version, etc.).  The server responds
// with HTTP 200 (OK) if the status message parses according to the definition in support/config.go,
// and HTTP 400 (Bad Request) if the body of the message fails to read or convert.  Any response should
// be used by the client to indicate that the server exists.  More sophisticated implementations might
// use the status information to update a local dB of logger status, health, etc.
func status_updates(w http.ResponseWriter, r *http.Request) {
	var body []byte
	var err error
	var status api.Status

	if body, err = io.ReadAll(r.Body); err != nil {
		support.Errorf("API: failed to read POST body component: %s\n", err)
		w.WriteHeader(http.StatusBadRequest)
		return
	}
	r.Body.Close()

	if err = json.Unmarshal(body, &status); err != nil {
		support.Errorf("API: failed to unmarshall request: %s\n", err)
		support.Errorf("API: body was |%s|\n", body)
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	support.Infof("CHECKIN: status update from logger with firmware %s, command processor %s, total %d files.\n",
		status.Versions.Firmware, status.Versions.CommandProcessor, status.Files.Count)
}

// Accept a file transfer from the logger client (which should contain a binary-encoded body
// with the WIBL raw file).  The client must specify the Content-Length header, the Digest header
// (with the MD5 hash of the contents of the body of the request), and the Authentication header
// with type "Basic" and the upload token specified by the server's operator when the logger was
// configured as a (very simple, and not terribly secure, identification mechanism).  The server
// responds with a JSON body containing only a "status" tag with either "success" or "failure" as
// appropriate.  Typical verification models would include checking the upload token from the
// Authentication header is one of those that was pre-shared, recomputing the MD5 hash for the
// payload and comparing it against that specified in the Digest header, etc.  A full implementation
// of the server would take the payload body, then transfer it to the appropriate S3 bucket for
// processing (using a UUID4 for the name), and finally trigger the SNS topic indicating that the
// file was ready for processing.
func file_transfer(w http.ResponseWriter, r *http.Request) {
	var body []byte
	var err error
	var result api.TransferResult

	support.Infof("TRANS: File transfer request with headers:\n")
	for k, v := range r.Header {
		support.Infof("TRANS:    %s = %s\n", k, v)
	}
	if body, err = io.ReadAll(r.Body); err != nil {
		support.Errorf("API: failed to read file body from POST: %s.\n", err)
		w.WriteHeader(http.StatusBadRequest)
		return
	}
	r.Body.Close()
	support.Infof("TRANS: File from logger with %d bytes in body.\n", len(body))
	md5digest := r.Header.Get("Digest")
	if len(md5digest) == 0 {
		support.Errorf("API: no digest in headers for file transfer.\n")
		w.WriteHeader(http.StatusBadRequest)
		return
	} else {
		md5digest = strings.Split(md5digest, "=")[1]
		support.Infof("TRANS: MD5 Digest |%s|\n", md5digest)
	}
	md5hash := fmt.Sprintf("%X", md5.Sum(body))
	if md5hash != md5digest {
		support.Errorf("API: recomputed MD5 digest doesn't match that sent from logger (%s != %s).\n",
			md5digest, md5hash)
		result.Status = "failure"
	} else {
		support.Infof("TRANS: successful recomputation of MD5 hash for transmitted contents.\n")
		result.Status = "success"
		// TODO: Further transfer of the file:
		//    1. Make a UUID for the transferred data.
		//    2. Store the received data into the appropriate S3 bucket for the current instance
		//       with the UUID.wibl extension.
		//    3. Trigger SNS topic for new file arrival.
	}
	w.Header().Set("Content-Type", "application/json")
	var result_string []byte
	if result_string, err = json.Marshal(result); err != nil {
		support.Errorf("API: failed to marshal response as JSON for file upload: %s\n", err)
		return
	}
	support.Infof("TRANS: sending |%s| to logger as response.\n", result_string)
	w.Write(result_string)
}
