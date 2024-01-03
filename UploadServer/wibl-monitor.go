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
	http.HandleFunc("/", syntax)
	http.HandleFunc("/checkin", status_updates)
	http.HandleFunc("/update", file_transfer)
	address := fmt.Sprintf(":%d", config.API.Port)
	log.Fatal(http.ListenAndServe(address, nil))

}

func syntax(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "checkin\n")
	fmt.Fprintf(w, "update\n")
}

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

func file_transfer(w http.ResponseWriter, r *http.Request) {
	var body []byte
	var err error
	var result api.TransferResult

	support.Infof("TRANS: File transfer request with headers:\n")
	for k, v := range r.Header {
		support.Infof("TRANS: %s = %s\n", k, v)
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
	}
	w.Header().Set("Content-Type", "application/json")
	var result_string []byte
	if result_string, err = json.Marshal(result); err != nil {
		support.Errorf("API: failed to marshal response as JSON for file upload: %s\n", err)
		return
	}
	support.Infof("TRANS: sending |%s| to logger as response.\n", result_string)
	w.Write(result_string)
	support.Infof("TRANS: upload transaction complete.\n")
}
