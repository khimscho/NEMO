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
	log.Fatal(http.ListenAndServe(fmt.Sprintf("localhost:%d", config.API.Port), nil))

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
		w.WriteHeader(http.StatusBadRequest)
		return
	}

	support.Infof("INF: status update from logger with firmware %s, command processor %s, total %d files.\n",
		status.Versions.Firmware, status.Versions.CommandProcessor, status.Files.Count)
}

func file_transfer(w http.ResponseWriter, r *http.Request) {
	var body []byte
	var err error
	var result api.TransferResult

	if body, err = io.ReadAll(r.Body); err != nil {
		support.Errorf("API: failed to read file body from POST: %s.\n", err)
		w.WriteHeader(http.StatusBadRequest)
		return
	}
	r.Body.Close()
	md5digest := r.Header.Get("Digest")
	if len(md5digest) == 0 {
		support.Errorf("API: no digest in headers for file transfer.\n")
		w.WriteHeader(http.StatusBadRequest)
		return
	}
	md5hash := fmt.Sprintf("%x", md5.Sum(body))
	if md5hash != md5digest {
		support.Errorf("API: recomputed MD5 digest doesn't match that sent from logger (%s != %s).\n",
			md5digest, md5hash)
		result.Status = "failure"
	} else {
		result.Status = "success"
	}
	w.Header().Set("Content-Type", "application/json")
	var result_string []byte
	if result_string, err = json.Marshal(result); err != nil {
		support.Errorf("API: failed to marshal response as JSON for file upload: %s\n", err)
		return
	}
	w.Write(result_string)
}
