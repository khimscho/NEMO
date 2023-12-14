package support

import (
	"encoding/json"
	"io"
	"os"
)

type APIParam struct {
	Port int `json:"port"`
}

type Config struct {
	API APIParam `json:"api"`
}

func NewConfig(filename string) (*Config, error) {
	config := new(Config)
	f, err := os.Open(filename)
	if err != nil {
		Errorf("failed to open %q for JSON configuration\n", filename)
		return nil, err
	}
	decoder := json.NewDecoder(f)
	if err := decoder.Decode(config); err != nil && err != io.EOF {
		Errorf("failed to decode JSON parameters from %q (%v)\n", filename, err)
		return nil, err
	}
	return config, nil
}

func NewDefaultConfig() *Config {
	config := new(Config)
	config.API.Port = 8000
	return config
}
